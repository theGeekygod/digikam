/* ============================================================
 * Author: Gilles Caulier <caulier dot gilles at kdemail dot net>
 * Date  : 2006-02-23
 * Description : image metadata interface
 *
 * Copyright 2006 by Gilles Caulier
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * ============================================================ */

 // C++ includes.

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <string>

// Qt includes.

#include <qfile.h>
#include <qwmatrix.h>

// KDE includes.

#include <kdebug.h>

// Exiv2 includes.

#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <exiv2/iptc.hpp>
#include <exiv2/tags.hpp>
#include <exiv2/datasets.hpp>

// Local includes.

#include "dcraw_parse.h"
#include "jpegmetaloader.h"
#include "pngmetaloader.h"
#include "tiffmetaloader.h"
#include "rawmetaloader.h"
#include "dmetadata.h"

namespace Digikam
{

DMetadata::DMetadata(const QString& filePath, DImg::FORMAT ff)
{
    m_filePath = filePath;
    load(filePath, ff);
}

QByteArray DMetadata::getExif() const
{
    return m_exifMetadata;
}

QByteArray DMetadata::getIptc() const
{
    return m_iptcMetadata;
}
    
void DMetadata::setExif(const QByteArray& data)
{
    m_exifMetadata = data;
}

void DMetadata::setIptc(const QByteArray& data)
{
    m_iptcMetadata = data;
}

bool DMetadata::load(const QString& filePath, DImg::FORMAT ff)
{
    DImg::FORMAT format = ff;
    
    if (format == DImg::NONE)
        format = fileFormat(filePath);

    switch (format)
    {
        case(DImg::JPEG):
        {
            JPEGMetaLoader loader(this);
            return (loader.load(filePath));
            break;
        }
        case(DImg::PNG):
        {
            PNGMetaLoader loader(this);
            return (loader.load(filePath));
            break;
        }
        case(DImg::RAW):
        {
            RAWMetaLoader loader(this);
            return (loader.load(filePath));
            break;
        }
        case(DImg::TIFF):
        {
            TIFFMetaLoader loader(this);
            return (loader.load(filePath));
            break;
        }
        default:
        {
            return false;
            break;
        }
    }

    return false;
}

bool DMetadata::save(const QString& filePath, const QString& format)
{
    if (format.isEmpty())
        return false;

    QString frm = format.upper();

    if (frm == "JPEG" || frm == "JPG")
    {
        JPEGMetaLoader loader(this);
        return loader.save(filePath);
    }
    else if (frm == "PNG")
    {
        PNGMetaLoader loader(this);
        return loader.save(filePath);
    }
    else if (frm == "TIFF" || frm == "TIF")
    {
        TIFFMetaLoader loader(this);
        return loader.save(filePath);
    }

    return false;
}

DImg::FORMAT DMetadata::fileFormat(const QString& filePath)
{
    if ( filePath == QString::null )
        return DImg::NONE;

    FILE* f = fopen(QFile::encodeName(filePath), "rb");
    
    if (!f)
    {
        kdDebug() << k_funcinfo << "Failed to open file" << endl;
        return DImg::NONE;
    }
    
    const int headerLen = 8;
    unsigned char header[headerLen];
    
    if (fread(&header, 8, 1, f) != 1)
    {
        kdDebug() << k_funcinfo << "Failed to read header" << endl;
        fclose(f);
        return DImg::NONE;
    }
    
    fclose(f);
    
    DcrawParse rawFileParser;
    uchar jpegID[2]    = { 0xFF, 0xD8 };   
    uchar tiffBigID[2] = { 0x4D, 0x4D };
    uchar tiffLilID[2] = { 0x49, 0x49 };
    uchar pngID[8]     = {'\211', 'P', 'N', 'G', '\r', '\n', '\032', '\n'};
    
    if (memcmp(&header, &jpegID, 2) == 0)            // JPEG file ?
    {
        return DImg::JPEG;
    }
    else if (memcmp(&header, &pngID, 8) == 0)        // PNG file ?
    {
        return DImg::PNG;
    }
    else if (rawFileParser.getCameraModel( QFile::encodeName(filePath), NULL, NULL) == 0)
    {
        // RAW File test using dcraw.  
        // Need to test it before TIFF because any RAW file 
        // formats using TIFF header.
        return DImg::RAW;
    }
    else if (memcmp(&header, &tiffBigID, 2) == 0 ||  // TIFF file ?
             memcmp(&header, &tiffLilID, 2) == 0)
    {
        return DImg::TIFF;
    }
    
    return DImg::NONE;
}

QImage DMetadata::getExifThumbnail(bool fixOrientation) const
{
    QImage thumbnail;
    
    if (m_exifMetadata.isEmpty())
       return thumbnail;
    
    try
    {    
        Exiv2::ExifData exifData;
        exifData.load((const Exiv2::byte*)m_exifMetadata.data(), m_exifMetadata.size());
        Exiv2::DataBuf const c1(exifData.copyThumbnail());
        thumbnail.loadFromData(c1.pData_, c1.size_);
        
        if (!thumbnail.isNull())
        {
            if (fixOrientation)
            {
                Exiv2::ExifKey key("Exif.Image.Orientation");
                Exiv2::ExifData::iterator it = exifData.findKey(key);
                if (it != exifData.end())
                {
                    QWMatrix matrix;
                    long orientation = it->toLong();
                    kdDebug() << " Exif Orientation: " << orientation << endl;
                    
                    switch (orientation) 
                    {
                        case ORIENTATION_HFLIP:
                            matrix.scale(-1, 1);
                            break;
                    
                        case ORIENTATION_ROT_180:
                            matrix.rotate(180);
                            break;
                    
                        case ORIENTATION_VFLIP:
                            matrix.scale(1, -1);
                            break;
                    
                        case ORIENTATION_ROT_90_HFLIP:
                            matrix.scale(-1, 1);
                            matrix.rotate(90);
                            break;
                    
                        case ORIENTATION_ROT_90:
                            matrix.rotate(90);
                            break;
                    
                        case ORIENTATION_ROT_90_VFLIP:
                            matrix.scale(1, -1);
                            matrix.rotate(90);
                            break;
                    
                        case ORIENTATION_ROT_270:
                            matrix.rotate(270);
                            break;
                            
                        default:
                            break;
                    }
        
                    if ( orientation != ORIENTATION_NORMAL )
                        thumbnail = thumbnail.xForm( matrix );
                }
                    
                return thumbnail;
            }
        }
    }
    catch( Exiv2::Error &e )
    {
        kdDebug() << "Cannot parse Exif Thumbnail using Exiv2 (" 
                  << QString::fromLocal8Bit(e.what().c_str())
                  << ")" << endl;
    }        
    
    return thumbnail;
}

DMetadata::ImageOrientation DMetadata::getExifImageOrientation()
{
    if (m_exifMetadata.isEmpty())
       return ORIENTATION_UNSPECIFIED;

    try
    {    
        Exiv2::ExifData exifData;
        exifData.load((const Exiv2::byte*)m_exifMetadata.data(), m_exifMetadata.size());
        Exiv2::ExifKey key("Exif.Image.Orientation");
        Exiv2::ExifData::iterator it = exifData.findKey(key);
        
        if (it != exifData.end())
        {
            long orientation = it->toLong();
            kdDebug() << " Exif Orientation: " << orientation << endl;
            return (ImageOrientation)orientation;
        }
    }
    catch( Exiv2::Error &e )
    {
        kdDebug() << "Cannot parse Exif Orientation tag using Exiv2 (" 
                  << QString::fromLocal8Bit(e.what().c_str())
                  << ")" << endl;
    }        
    
    return ORIENTATION_UNSPECIFIED;
}

bool DMetadata::writeExifImageOrientation(const QString& filePath, ImageOrientation orientation)
{
    try
    {    
        if (filePath.isEmpty())
            return false;
            
        if (orientation < ORIENTATION_UNSPECIFIED || orientation > ORIENTATION_ROT_270)
        {
            kdDebug() << k_funcinfo << "Exif orientation tag value is not correct!" << endl;
            return false;
        }
            
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open((const char*)
                                      (QFile::encodeName(filePath)));
        
        image->readMetadata();
        Exiv2::ExifData &exifData = image->exifData();
        
        if (exifData.empty())
        {
            kdDebug() << "Cannot set Exif Orientation tag from " << filePath 
                      << " because there is no Exif informations available!" << endl;
        }
        else
        {
            exifData["Exif.Image.Orientation"] = (uint16_t)orientation;
            image->setExifData(exifData);
            image->writeMetadata();
            kdDebug() << "Exif orientation tag set to:" << orientation << endl;
            return true;
        }    
    }
    catch( Exiv2::Error &e )
    {
        kdDebug() << "Cannot set Exif Orientation tag using Exiv2 (" 
                  << QString::fromLocal8Bit(e.what().c_str())
                  << ")" << endl;
    }        
    
    return false;
}

QDateTime DMetadata::getDateTime() const
{
    try
    {    
        // In first, trying to get Date & time from Exif tags.
        
        if (!m_exifMetadata.isEmpty())
        {        
            Exiv2::ExifData exifData;
            exifData.load((const Exiv2::byte*)m_exifMetadata.data(), m_exifMetadata.size());
    
            // Try standard Exif date time entry.
    
            Exiv2::ExifKey key("Exif.Image.DateTime");
            Exiv2::ExifData::iterator it = exifData.findKey(key);
            
            if (it != exifData.end())
            {
                QDateTime dateTime = QDateTime::fromString(it->toString().c_str(), Qt::ISODate);
    
                if (dateTime.isValid())
                {
                    kdDebug() << "DateTime (Exif standard): " << dateTime << endl;
                    return dateTime;
                }
            }
    
            // Bogus standard Exif date time entry. Try Exif date time original.
    
            Exiv2::ExifKey key2("Exif.Photo.DateTimeOriginal");
            Exiv2::ExifData::iterator it2 = exifData.findKey(key2);
            
            if (it2 != exifData.end())
            {
                QDateTime dateTime = QDateTime::fromString(it2->toString().c_str(), Qt::ISODate);
    
                if (dateTime.isValid())
                {
                    kdDebug() << "DateTime (Exif original): " << dateTime << endl;
                    return dateTime;
                }
            }
    
            // Bogus Exif date time original entry. Try Exif date time digitized.
    
            Exiv2::ExifKey key3("Exif.Photo.DateTimeDigitized");
            Exiv2::ExifData::iterator it3 = exifData.findKey(key3);
            
            if (it3 != exifData.end())
            {
                QDateTime dateTime = QDateTime::fromString(it3->toString().c_str(), Qt::ISODate);
    
                if (dateTime.isValid())
                {
                    kdDebug() << "DateTime (Exif digitalized): " << dateTime << endl;
                    return dateTime;
                }
            }
        }
        
        // In second, trying to get Date & time from Iptc tags.
            
        if (!m_iptcMetadata.isEmpty())
        {        
            Exiv2::IptcData iptcData;
            iptcData.load((const Exiv2::byte*)m_iptcMetadata.data(), m_iptcMetadata.size());
            
            // Try creation Iptc date time entries.
    
            Exiv2::IptcKey keyDateCreated("Iptc.Application2.DateCreated");
            Exiv2::IptcData::iterator it = iptcData.findKey(keyDateCreated);
                        
            if (it != iptcData.end())
            {
                QString IptcDateCreated(it->toString().c_str());
    
                Exiv2::IptcKey keyTimeCreated("Iptc.Application2.TimeCreated");
                Exiv2::IptcData::iterator it2 = iptcData.findKey(keyTimeCreated);
                
                if (it2 != iptcData.end())
                {
                    QString IptcTimeCreated(it2->toString().c_str());
                    
                    QDate date = QDate::fromString(IptcDateCreated, Qt::ISODate);
                    QTime time = QTime::fromString(IptcTimeCreated, Qt::ISODate);
                    QDateTime dateTime = QDateTime(date, time);
                    
                    if (dateTime.isValid())
                    {
                        kdDebug() << "Date (IPTC created): " << dateTime << endl;
                        return dateTime;
                    }                    
                }
            }                        
            
            // Try digitization Iptc date time entries.
    
            Exiv2::IptcKey keyDigitizationDate("Iptc.Application2.DigitizationDate");
            Exiv2::IptcData::iterator it3 = iptcData.findKey(keyDigitizationDate);
                        
            if (it3 != iptcData.end())
            {
                QString IptcDateDigitization(it3->toString().c_str());
    
                Exiv2::IptcKey keyDigitizationTime("Iptc.Application2.DigitizationTime");
                Exiv2::IptcData::iterator it4 = iptcData.findKey(keyDigitizationTime);
                
                if (it4 != iptcData.end())
                {
                    QString IptcTimeDigitization(it4->toString().c_str());
                    
                    QDate date = QDate::fromString(IptcDateDigitization, Qt::ISODate);
                    QTime time = QTime::fromString(IptcTimeDigitization, Qt::ISODate);
                    QDateTime dateTime = QDateTime(date, time);
                    
                    if (dateTime.isValid())
                    {
                        kdDebug() << "Date (IPTC digitalized): " << dateTime << endl;
                        return dateTime;
                    }                    
                }
            }                       
        }
    }
    catch( Exiv2::Error &e )
    {
        kdDebug() << "Cannot parse Exif date tag using Exiv2 (" 
                  << QString::fromLocal8Bit(e.what().c_str())
                  << ")" << endl;
    }        
    
    return QDateTime();
}

QString DMetadata::getImageComment() const
{
    try
    {    
        if (m_filePath.isEmpty())
            return QString();
            
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open((const char*)
                                      (QFile::encodeName(m_filePath)));
        
        // In first we trying to get image comments, outside of Exif and IPTC.
                                              
        image->readMetadata();
        QString comment(image->comment().c_str());
        
        if (!comment.isEmpty())
           return comment;
           
        // In second, we trying to get Exif comments   
                
        if (m_exifMetadata.isEmpty())
        {
            Exiv2::ExifData exifData;
            exifData.load((const Exiv2::byte*)m_exifMetadata.data(), m_exifMetadata.size());
            Exiv2::ExifKey key("Exif.Photo.UserComment");
            Exiv2::ExifData::iterator it = exifData.findKey(key);
            
            if (it != exifData.end())
            {
                QString ExifComment(it->toString().c_str());
    
                if (!ExifComment.isEmpty())
                  return ExifComment;
            }
        }
        
        // In third, we trying to get IPTC comments   
                
        if (m_iptcMetadata.isEmpty())
        {
            Exiv2::IptcData iptcData;
            iptcData.load((const Exiv2::byte*)m_iptcMetadata.data(), m_iptcMetadata.size());
            Exiv2::IptcKey key("Iptc.Application2.Caption");
            Exiv2::IptcData::iterator it = iptcData.findKey(key);
            
            if (it != iptcData.end())
            {
                QString IptcComment(it->toString().c_str());
    
                if (!IptcComment.isEmpty())
                  return IptcComment;
            }
        }

    }
    catch( Exiv2::Error &e )
    {
        kdDebug() << "Cannot get Image comments using Exiv2 (" 
                  << QString::fromLocal8Bit(e.what().c_str())
                  << ")" << endl;
    }        
    
    return QString();
}

bool DMetadata::writeImageComment(const QString& filePath, const QString& comment)
{
    try
    {    
        if (filePath.isEmpty())
            return false;
            
        if (comment.isEmpty())
        {
            kdDebug() << k_funcinfo << "Comment to write is empty!" << endl;
            return false;
        }
            
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open((const char*)
                                      (QFile::encodeName(filePath)));
        
        // In first we write comments outside of Exif and IPTC if possible.
                                              
        image->readMetadata();
        const std::string &str(comment.latin1());
        image->setComment(str);

        // In Second we write comments into Exif.
                
        Exiv2::ExifData &exifData = image->exifData();
        exifData["Exif.Photo.UserComment"] = comment.latin1();
        image->setExifData(exifData);
        
        // In Third we write comments into Iptc.

        Exiv2::IptcData &iptcData = image->iptcData();
        iptcData["Iptc.Application2.Caption"] = comment.latin1();
        image->setIptcData(iptcData);
    
        image->writeMetadata();
        return true;
    }
    catch( Exiv2::Error &e )
    {
        kdDebug() << "Cannot set Comment into image using Exiv2 (" 
                  << QString::fromLocal8Bit(e.what().c_str())
                  << ")" << endl;
    }        
    
    return false;
}

int DMetadata::getImageRating() const
{
    try
    {    
        if (m_filePath.isEmpty())
            return -1;
            
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open((const char*)
                                      (QFile::encodeName(m_filePath)));
        
        image->readMetadata();
        
        if (m_iptcMetadata.isEmpty())
        {
            Exiv2::IptcData iptcData;
            iptcData.load((const Exiv2::byte*)m_iptcMetadata.data(), m_iptcMetadata.size());
            Exiv2::IptcKey key("Iptc.Application2.FixtureId");
            Exiv2::IptcData::iterator it = iptcData.findKey(key);
            
            if (it != iptcData.end())
            {
                QString IptcComment(it->toString().c_str());
    
                if (IptcComment == QString("digiKam Rating=0"))
                    return 0;
                else if (IptcComment == QString("digiKam Rating=1"))
                    return 1;
                else if (IptcComment == QString("digiKam Rating=2"))
                    return 2;
                else if (IptcComment == QString("digiKam Rating=3"))
                    return 3;
                else if (IptcComment == QString("digiKam Rating=4"))
                    return 4;
                else if (IptcComment == QString("digiKam Rating=5"))
                    return 5;
            }
        }
    }
    catch( Exiv2::Error &e )
    {
        kdDebug() << "Cannot get Image rating using Exiv2 (" 
                  << QString::fromLocal8Bit(e.what().c_str())
                  << ")" << endl;
    }        
    
    return -1;
}

bool DMetadata::writeImageRating(const QString& filePath, int rating)
{
    try
    {    
        if (filePath.isEmpty())
            return false;
            
        if (rating < 0 || rating > 5)
        {
            kdDebug() << k_funcinfo << "Rating value to write out of range!" << endl;
            return false;
        }
            
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open((const char*)
                                      (QFile::encodeName(filePath)));
        
        image->readMetadata();
        Exiv2::IptcData &iptcData = image->iptcData();
        
        QString temp;
        QString ratingString("digiKam Rating=");
        ratingString.append(temp.setNum(rating));
        
        iptcData["Iptc.Application2.FixtureId"] = ratingString.latin1();
        image->setIptcData(iptcData);
        image->writeMetadata();
        return true;
    }
    catch( Exiv2::Error &e )
    {
        kdDebug() << "Cannot set Comment into image using Exiv2 (" 
                  << QString::fromLocal8Bit(e.what().c_str())
                  << ")" << endl;
    }        
    
    return false;
}

}  // NameSpace Digikam
