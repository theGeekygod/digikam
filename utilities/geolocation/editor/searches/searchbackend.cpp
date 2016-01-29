/** ===========================================================
 * @file
 *
 * This file is a part of digiKam project
 * <a href="http://www.digikam.org">http://www.digikam.org</a>
 *
 * @date   2010-06-01
 * @brief  A simple backend to search OSM and Geonames.org.
 *
 * @author Copyright (C) 2010, 2011 by Michael G. Hansen
 *         <a href="mailto:mike at mghansen dot de">mike at mghansen dot de</a>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "searchbackend.h"

// Qt includes

#include <QDomDocument>
#include <QUrlQuery>

// KDE includes

#include <kio/job.h>
#include <klocalizedstring.h>

// local includes

#include "gpscommon.h"

namespace Digikam
{

class SearchBackend::Private
{
public:
    Private()
      : results(),
        kioJob(0),
        runningBackend(),
        searchData(),
        errorMessage()
    {
    }

    SearchBackend::SearchResult::List results;
    KIO::Job*                         kioJob;
    QString                           runningBackend;
    QByteArray                        searchData;
    QString                           errorMessage;
};

SearchBackend::SearchBackend(QObject* const parent)
    : QObject(parent), d(new Private())
{
}

SearchBackend::~SearchBackend()
{
    delete d;
}

bool SearchBackend::search(const QString& backendName, const QString& searchTerm) const
{
    d->searchData.clear();
    d->errorMessage.clear();
    d->results.clear();

    if (backendName == QString::fromLatin1("osm"))
    {
        d->runningBackend = backendName;

        QUrl jobUrl(QString::fromLatin1("http://nominatim.openstreetmap.org/search"));
        
        QUrlQuery q(jobUrl);
        q.addQueryItem(QString::fromLatin1("format"), QString::fromLatin1("xml"));
        q.addQueryItem(QString::fromLatin1("q"), searchTerm);
        jobUrl.setQuery(q);
        
        d->kioJob = KIO::get(jobUrl, KIO::NoReload, KIO::HideProgressInfo);
        d->kioJob->addMetaData(QString::fromLatin1("User-Agent"), getUserAgentName());

        connect(d->kioJob, SIGNAL(data(KIO::Job*,QByteArray)),
                this, SLOT(slotData(KIO::Job*,QByteArray)));

        connect(d->kioJob, SIGNAL(result(KJob*)),
                this, SLOT(slotResult(KJob*)));

        return true;
    }

    if (backendName == QString::fromLatin1("geonames.org"))
    {
        d->runningBackend = backendName;

        // documentation: http://www.geonames.org/export/geonames-search.html

        QUrl jobUrl(QString::fromLatin1("http://ws.geonames.org/search"));
        
        QUrlQuery q(jobUrl);
        q.addQueryItem(QString::fromLatin1("type"), QString::fromLatin1("xml"));
        q.addQueryItem(QString::fromLatin1("q"), searchTerm);
        jobUrl.setQuery(q);

        d->kioJob = KIO::get(jobUrl, KIO::NoReload, KIO::HideProgressInfo);
        d->kioJob->addMetaData(QString::fromLatin1("User-Agent"), getUserAgentName());

        connect(d->kioJob, SIGNAL(data(KIO::Job*,QByteArray)),
                this, SLOT(slotData(KIO::Job*,QByteArray)));

        connect(d->kioJob, SIGNAL(result(KJob*)),
                this, SLOT(slotResult(KJob*)));

        return true;
    }

    return false;
}

void SearchBackend::slotData(KIO::Job* kioJob, const QByteArray& data)
{
    Q_UNUSED(kioJob)

    d->searchData.append(data);
}

void SearchBackend::slotResult(KJob* kJob)
{
    if (kJob != d->kioJob)
    {
        return;
    }

    if (d->kioJob->error())
    {
        d->errorMessage = d->kioJob->errorString();
        emit(signalSearchCompleted());
        return;
    }

    const QString resultString = QString::fromUtf8(d->searchData.constData(), d->searchData.count());

    if (d->runningBackend == QString::fromLatin1("osm"))
    {
        QDomDocument doc;
        doc.setContent(resultString); // error-handling
        QDomElement docElement = doc.documentElement(); // error-handling

        for (QDomNode resultNode = docElement.firstChild(); !resultNode.isNull(); resultNode = resultNode.nextSibling())
        {
            QDomElement resultElement = resultNode.toElement();

            if (resultElement.isNull())
            {
                continue;
            }

            if (resultElement.tagName() != QString::fromLatin1("place"))
            {
                continue;
            }

            const QString boundingBoxString = resultElement.attribute(QString::fromLatin1("boundingbox"));
            const QString latString         = resultElement.attribute(QString::fromLatin1("lat"));
            const QString lonString         = resultElement.attribute(QString::fromLatin1("lon"));
            const QString displayName       = resultElement.attribute(QString::fromLatin1("display_name"));
            const QString placeId           = resultElement.attribute(QString::fromLatin1("place_id"));

            if (latString.isEmpty() || lonString.isEmpty() || displayName.isEmpty())
            {
                continue;
            }

            // now parse the strings:
            qreal lat;
            qreal lon;
            bool okay = false;
            lat       = latString.toDouble(&okay);

            if (okay)
            {
                lon = lonString.toDouble(&okay);
            }

            if (!okay)
            {
                continue;
            }

            SearchResult result;
            result.coordinates = GeoIface::GeoCoordinates(lat, lon);
            result.name        = displayName;

            if (!placeId.isEmpty())
            {
                result.internalId = QString::fromLatin1("osm-") + placeId;
            }

            // TODO: parse bounding box

            d->results << result;
        }
    }
    else if (d->runningBackend == QString::fromLatin1("geonames.org"))
    {
        QDomDocument doc;
        doc.setContent(resultString); // error-handling
        QDomElement docElement = doc.documentElement(); // error-handling
        qCDebug(DIGIKAM_GENERAL_LOG)<<docElement.toElement().tagName();

        for (QDomNode resultNode = docElement.firstChild(); !resultNode.isNull(); resultNode = resultNode.nextSibling())
        {
            QDomElement resultElement = resultNode.toElement();
            qCDebug(DIGIKAM_GENERAL_LOG) << resultElement.tagName();

            if (resultElement.isNull())
            {
                continue;
            }

            if (resultElement.tagName() != QString::fromLatin1("geoname"))
            {
                continue;
            }

            QString latString;
            QString lonString;
            QString displayName;
            QString geoNameId;

            for (QDomNode resultSubNode = resultElement.firstChild(); !resultSubNode.isNull(); resultSubNode = resultSubNode.nextSibling())
            {
                QDomElement resultSubElement = resultSubNode.toElement();

                if (resultSubElement.isNull())
                {
                    continue;
                }

                if (resultSubElement.tagName() == QString::fromLatin1("lat"))
                {
                    latString = resultSubElement.text();
                }
                else if (resultSubElement.tagName() == QString::fromLatin1("lng"))
                {
                    lonString = resultSubElement.text();
                }
                else if (resultSubElement.tagName() == QString::fromLatin1("name"))
                {
                    displayName = resultSubElement.text();
                }
                else if (resultSubElement.tagName() == QString::fromLatin1("geonameId"))
                {
                    geoNameId = resultSubElement.text();
                }
            }

            if (latString.isEmpty()||lonString.isEmpty()||displayName.isEmpty())
            {
                continue;
            }

            // now parse the strings:
            qreal lat;
            qreal lon;
            bool okay = false;
            lat       = latString.toDouble(&okay);

            if (okay)
            {
                lon = lonString.toDouble(&okay);
            }

            if (!okay)
            {
                continue;
            }

            SearchResult result;
            result.coordinates = GeoIface::GeoCoordinates(lat, lon);
            result.name        = displayName;

            if (!geoNameId.isEmpty())
            {
                result.internalId = QString::fromLatin1("geonames.org-") + geoNameId;
            }

            d->results << result;
        }
    }

    emit(signalSearchCompleted());
}

SearchBackend::SearchResult::List SearchBackend::getResults() const
{
    return d->results;
}

QString SearchBackend::getErrorMessage() const
{
    return d->errorMessage;
}

QList<QPair<QString, QString> > SearchBackend::getBackends() const
{
    QList<QPair<QString, QString> > resultList;
    resultList << QPair<QString, QString>(i18n("GeoNames"), QString::fromLatin1("geonames.org"));
    resultList << QPair<QString, QString>(i18n("OSM"), QString::fromLatin1("osm"));

    return resultList;
}

} /* namespace Digikam */
