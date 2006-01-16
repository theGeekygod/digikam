/* ============================================================
 * File  : adjustlevels.cpp
 * Author: Gilles Caulier <caulier dot gilles at free.fr>
 * Date  : 2004-07-20
 * Description : image histogram adjust levels.
 *
 * Copyright 2004-2006 by Gilles Caulier
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

#include <cmath>
#include <cstring>

// Qt includes.

#include <qlayout.h>
#include <qcolor.h>
#include <qgroupbox.h>
#include <qhgroupbox.h>
#include <qvgroupbox.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qwhatsthis.h>
#include <qtooltip.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qframe.h>
#include <qtimer.h>
#include <qhbuttongroup.h> 
#include <qpixmap.h>
#include <qcheckbox.h>

// KDE includes.

#include <kcursor.h>
#include <kdebug.h>
#include <klocale.h>
#include <knuminput.h>
#include <kmessagebox.h>
#include <kselect.h>
#include <kfiledialog.h>
#include <kglobalsettings.h>
#include <kaboutdata.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <kapplication.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>

// Local includes.

#include "version.h"
#include "adjustlevels.h"

namespace DigikamAdjustLevelsImagesPlugin
{

AdjustLevelDialog::AdjustLevelDialog(QWidget* parent, QString title, QFrame* banner)
                 : ImageTabDialog(parent, title, "adjustlevels", 
                                  true, true, true, banner)
{
    m_destinationPreviewData = 0L;

    Digikam::ImageIface iface(0, 0);
    uchar *data     = iface.getOriginalImage();
    int w           = iface.originalWidth();
    int h           = iface.originalHeight();
    bool sixteenBit = iface.originalSixteenBit();
    bool hasAlpha   = iface.originalHasAlpha();
    m_originalImage = Digikam::DImg(w, h, sixteenBit, hasAlpha ,data);
    delete [] data;

    m_histoSegments = m_originalImage.sixteenBit() ? 65535 : 255;
    m_levels = new Digikam::ImageLevels(m_originalImage.sixteenBit());

    // About data and help button.

    KAboutData* about = new KAboutData("digikamimageplugins",
                                       I18N_NOOP("Adjust Color Levels"),
                                       digikamimageplugins_version,
                                       I18N_NOOP("An image-histogram-levels adjustment plugin for digiKam."),
                                       KAboutData::License_GPL,
                                       "(c) 2004-2006, Gilles Caulier",
                                       0,
                                       "http://extragear.kde.org/apps/digikamimageplugins");

    about->addAuthor("Gilles Caulier", I18N_NOOP("Author and maintainer"),
                     "caulier dot gilles at free.fr");

    setAboutData(about);
    
    // -------------------------------------------------------------

    QWidget *gboxSettings = new QWidget(plainPage());
    QGridLayout* grid = new QGridLayout( gboxSettings, 10, 5, marginHint(), spacingHint());

    QLabel *label1 = new QLabel(i18n("Channel:"), gboxSettings);
    label1->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    m_channelCB = new QComboBox( false, gboxSettings );
    m_channelCB->insertItem( i18n("Luminosity") );
    m_channelCB->insertItem( i18n("Red") );
    m_channelCB->insertItem( i18n("Green") );
    m_channelCB->insertItem( i18n("Blue") );
    m_channelCB->insertItem( i18n("Alpha") );
    m_channelCB->setCurrentText( i18n("Luminosity") );
    QWhatsThis::add( m_channelCB, i18n("<p>Select here the histogram channel to display:<p>"
                                       "<b>Luminosity</b>: display the image's luminosity values.<p>"
                                       "<b>Red</b>: display the red image-channel values.<p>"
                                       "<b>Green</b>: display the green image-channel values.<p>"
                                       "<b>Blue</b>: display the blue image-channel values.<p>"
                                       "<b>Alpha</b>: display the alpha image-channel values. "
                                       "This channel corresponds to the transparency value and "
                                       "is supported by some image formats, such as PNG or GIF."));

    m_scaleBG = new QHButtonGroup(gboxSettings);
    m_scaleBG->setExclusive(true);
    m_scaleBG->setFrameShape(QFrame::NoFrame);
    m_scaleBG->setInsideMargin( 0 );
    QWhatsThis::add( m_scaleBG, i18n("<p>Select here the histogram scale.<p>"
                                     "If the image's maximal counts are small, you can use the linear scale.<p>"
                                     "Logarithmic scale can be used when the maximal counts are big; "
                                     "if it is used, all values (small and large) will be visible on the graph."));
    
    QPushButton *linHistoButton = new QPushButton( m_scaleBG );
    QToolTip::add( linHistoButton, i18n( "<p>Linear" ) );
    m_scaleBG->insert(linHistoButton, Digikam::HistogramWidget::LinScaleHistogram);
    KGlobal::dirs()->addResourceType("histogram-lin", KGlobal::dirs()->kde_default("data") + "digikam/data");
    QString directory = KGlobal::dirs()->findResourceDir("histogram-lin", "histogram-lin.png");
    linHistoButton->setPixmap( QPixmap( directory + "histogram-lin.png" ) );
    linHistoButton->setToggleButton(true);
    
    QPushButton *logHistoButton = new QPushButton( m_scaleBG );
    QToolTip::add( logHistoButton, i18n( "<p>Logarithmic" ) );
    m_scaleBG->insert(logHistoButton, Digikam::HistogramWidget::LogScaleHistogram);
    KGlobal::dirs()->addResourceType("histogram-log", KGlobal::dirs()->kde_default("data") + "digikam/data");
    directory = KGlobal::dirs()->findResourceDir("histogram-log", "histogram-log.png");
    logHistoButton->setPixmap( QPixmap( directory + "histogram-log.png" ) );
    logHistoButton->setToggleButton(true);       

    QHBoxLayout* l1 = new QHBoxLayout();
    l1->addWidget(label1);
    l1->addWidget(m_channelCB);
    l1->addWidget(m_scaleBG);
    l1->addStretch(10);
    
    grid->addMultiCellLayout(l1, 0, 0, 0, 3);

    // -------------------------------------------------------------

    m_histogramWidget = new Digikam::HistogramWidget(256, 140, gboxSettings, false, true, true);
    QWhatsThis::add( m_histogramWidget, i18n("<p>Here you can see the target preview image histogram drawing of the "
                                             "selected image channel. This one is re-computed at any levels "
                                             "settings changes."));
    
    grid->addMultiCellWidget(m_histogramWidget, 1, 1, 0, 4);
    
    // -------------------------------------------------------------

    m_levelsHistogramWidget = new Digikam::HistogramWidget(256, 140, m_originalImage.bits(), m_originalImage.width(),
                                                     m_originalImage.height(), m_originalImage.sixteenBit(), gboxSettings, false);
    QWhatsThis::add( m_levelsHistogramWidget, i18n("<p>This is the histogram drawing of the selected channel from original image"));
    grid->addMultiCellWidget(m_levelsHistogramWidget, 2, 2, 0, 4);

    // -------------------------------------------------------------
    
    m_hGradientMinInput = new KGradientSelector( KSelector::Horizontal, gboxSettings );
    m_hGradientMinInput->setFixedHeight( 20 );
    m_hGradientMinInput->setMinValue(0);
    m_hGradientMinInput->setMaxValue(m_histoSegments);
    QWhatsThis::add( m_hGradientMinInput, i18n("<p>Select here the minimal intensity input value of the histogram."));
    QToolTip::add( m_hGradientMinInput, i18n( "Minimal intensity input." ) );
    m_hGradientMinInput->setColors( QColor( "black" ), QColor( "white" ) );
    grid->addMultiCellWidget(m_hGradientMinInput, 3, 3, 0, 4);

    m_hGradientMaxInput = new KGradientSelector( KSelector::Horizontal, gboxSettings );
    m_hGradientMaxInput->setFixedHeight( 20 );
    m_hGradientMaxInput->setMinValue(0);
    m_hGradientMaxInput->setMaxValue(m_histoSegments);
    QWhatsThis::add( m_hGradientMaxInput, i18n("<p>Select here the maximal intensity input value of the histogram."));
    QToolTip::add( m_hGradientMaxInput, i18n( "Maximal intensity input." ) );
    m_hGradientMaxInput->setColors( QColor( "black" ), QColor( "white" ) );
    grid->addMultiCellWidget(m_hGradientMaxInput, 4, 4, 0, 4);

    m_minInput = new QSpinBox(0, m_histoSegments, 1, gboxSettings);
    m_minInput->setValue(0);
    QWhatsThis::add( m_minInput, i18n("<p>Select here the minimal intensity input value of the histogram."));
    QToolTip::add( m_minInput, i18n( "Minimal intensity input." ) );
    m_gammaInput = new KDoubleNumInput(gboxSettings);
    m_gammaInput->setPrecision(2);
    m_gammaInput->setRange(0.1, 10.0, 0.1);
    m_gammaInput->setValue(1.0);
    QToolTip::add( m_gammaInput, i18n( "Gamma input value." ) );
    QWhatsThis::add( m_gammaInput, i18n("<p>Select here the gamma input value."));
    m_maxInput = new QSpinBox(0, m_histoSegments, 1, gboxSettings);
    m_maxInput->setValue(m_histoSegments);
    QToolTip::add( m_maxInput, i18n( "Maximal intensity input." ) );
    QWhatsThis::add( m_maxInput, i18n("<p>Select here the maximal intensity input value of the histogram."));
    grid->addMultiCellWidget(m_minInput, 3, 3, 5, 5);
    grid->addMultiCellWidget(m_maxInput, 4, 4, 5, 5);
    grid->addMultiCellWidget(m_gammaInput, 5, 5, 0, 5);

    m_hGradientMinOutput = new KGradientSelector( KSelector::Horizontal, gboxSettings );
    m_hGradientMinOutput->setColors( QColor( "black" ), QColor( "white" ) );
    QWhatsThis::add( m_hGradientMinOutput, i18n("<p>Select here the minimal intensity output value of the histogram."));    
    QToolTip::add( m_hGradientMinOutput, i18n( "Minimal intensity output." ) );
    m_hGradientMinOutput->setFixedHeight( 20 );
    m_hGradientMinOutput->setMinValue(0);
    m_hGradientMinOutput->setMaxValue(m_histoSegments);
    grid->addMultiCellWidget(m_hGradientMinOutput, 6, 6, 0, 4);

    m_hGradientMaxOutput = new KGradientSelector( KSelector::Horizontal, gboxSettings );
    m_hGradientMaxOutput->setColors( QColor( "black" ), QColor( "white" ) );
    QWhatsThis::add( m_hGradientMaxOutput, i18n("<p>Select here the maximal intensity output value of the histogram."));
    QToolTip::add( m_hGradientMaxOutput, i18n( "Maximal intensity output." ) );
    m_hGradientMaxOutput->setFixedHeight( 20 );
    m_hGradientMaxOutput->setMinValue(0);
    m_hGradientMaxOutput->setMaxValue(m_histoSegments);
    grid->addMultiCellWidget(m_hGradientMaxOutput, 7, 7, 0, 4);

    m_minOutput = new QSpinBox(0, m_histoSegments, 1, gboxSettings);
    m_minOutput->setValue(0);
    QToolTip::add( m_minOutput, i18n( "Minimal intensity output." ) );
    QWhatsThis::add( m_minOutput, i18n("<p>Select here the minimal intensity output value of the histogram."));
    m_maxOutput = new QSpinBox(0, m_histoSegments, 1, gboxSettings);
    m_maxOutput->setValue(m_histoSegments);
    QToolTip::add( m_maxOutput, i18n( "Maximal intensity output." ) );
    QWhatsThis::add( m_maxOutput, i18n("<p>Select here the maximal intensity output value of the histogram."));
    grid->addMultiCellWidget(m_minOutput, 6, 6, 5, 5);
    grid->addMultiCellWidget(m_maxOutput, 7, 7, 5, 5);

    // -------------------------------------------------------------

    m_pickerColorButtonGroup = new QHButtonGroup(gboxSettings);
    m_pickBlack = new QPushButton(m_pickerColorButtonGroup);
    m_pickerColorButtonGroup->insert(m_pickBlack, BlackTonal);
    KGlobal::dirs()->addResourceType("color-picker-black", KGlobal::dirs()->kde_default("data") +
                                     "digikamimageplugins/data");
    directory = KGlobal::dirs()->findResourceDir("color-picker-black", "color-picker-black.png");
    m_pickBlack->setPixmap( QPixmap( directory + "color-picker-black.png" ) );
    m_pickBlack->setToggleButton(true);
    QToolTip::add( m_pickBlack, i18n( "All channels shadow tone color picker" ) );
    QWhatsThis::add( m_pickBlack, i18n("<p>With this button, you can pick the color from original image used to set <b>Shadow Tone</b> "
                                       "levels input on Red, Green, Blue, and Luminosity channels."));
    m_pickGray  = new QPushButton(m_pickerColorButtonGroup);
    m_pickerColorButtonGroup->insert(m_pickGray, GrayTonal);
    KGlobal::dirs()->addResourceType("color-picker-gray", KGlobal::dirs()->kde_default("data") +
                                     "digikamimageplugins/data");
    directory = KGlobal::dirs()->findResourceDir("color-picker-gray", "color-picker-gray.png");
    m_pickGray->setPixmap( QPixmap( directory + "color-picker-gray.png" ) );
    m_pickGray->setToggleButton(true);
    QToolTip::add( m_pickGray, i18n( "All channels middle tone color picker" ) );
    QWhatsThis::add( m_pickGray, i18n("<p>With this button, you can pick the color from original image used to set <b>Middle Tone</b> "
                                      "levels input on Red, Green, Blue, and Luminosity channels."));
    m_pickWhite = new QPushButton(m_pickerColorButtonGroup);
    m_pickerColorButtonGroup->insert(m_pickWhite, WhiteTonal);
    KGlobal::dirs()->addResourceType("color-picker-white", KGlobal::dirs()->kde_default("data") +
                                     "digikamimageplugins/data");
    directory = KGlobal::dirs()->findResourceDir("color-picker-white", "color-picker-white.png");
    m_pickWhite->setPixmap( QPixmap( directory + "color-picker-white.png" ) );
    m_pickWhite->setToggleButton(true);
    QToolTip::add( m_pickWhite, i18n( "All channels highlight tone color picker" ) );
    QWhatsThis::add( m_pickWhite, i18n("<p>With this button, you can pick the color from original image used to set <b>Highlight Tone</b> "
                                       "levels input on Red, Green, Blue, and Luminosity channels."));
    m_pickerColorButtonGroup->setExclusive(true);
    m_pickerColorButtonGroup->setFrameShape(QFrame::NoFrame);    

    KIconLoader icon;
    m_autoButton = new QPushButton(gboxSettings);
    m_autoButton->setPixmap( icon.loadIcon( "run", (KIcon::Group)KIcon::Toolbar ) );
    QToolTip::add( m_autoButton, i18n( "Adjust all levels automatically." ) );
    QWhatsThis::add( m_autoButton, i18n("<p>If you press this button, all channel levels will be adjusted "
                                        "automatically."));

    m_resetButton = new QPushButton(i18n("&Reset"), gboxSettings);
    m_resetButton->setPixmap( icon.loadIcon( "locationbar_erase", (KIcon::Group)KIcon::Toolbar ) );
    QToolTip::add( m_resetButton, i18n( "Reset current channel levels' values." ) );
    QWhatsThis::add( m_resetButton, i18n("<p>If you press this button, all levels' values from the current selected channel "
                                         "will be reset to the default values."));
    
    QHBoxLayout* l3 = new QHBoxLayout();
    l3->addWidget(m_pickerColorButtonGroup);
    l3->addWidget(m_autoButton);
    l3->addWidget(m_resetButton);
    l3->addStretch(10);
    
    grid->addMultiCellLayout(l3, 8, 8, 0, 4);

    // -------------------------------------------------------------
    
    m_overExposureIndicatorBox = new QCheckBox(i18n("Over exposure indicator"), gboxSettings);
    QWhatsThis::add( m_overExposureIndicatorBox, i18n("<p>If you enable this option, over-exposed pixels "
                                                      "from the target image preview will be over-colored. "
                                                      "This will not have an effect on the final rendering."));
    grid->addMultiCellWidget(m_overExposureIndicatorBox, 9, 9, 0, 5);
    grid->setRowStretch(10, 10);
    
    setUserAreaWidget(gboxSettings);    
    
    // -------------------------------------------------------------

    m_previewOriginalWidget = previewOriginalWidget();
    QWhatsThis::add( m_previewOriginalWidget, i18n("<p>Here you can see the original image. You can pick "
                                                   "a color from the image using the color "
                                                   "picker tools to select shadow, middle, and highlight "
                                                   "tones to adjust the curves' points in the Red, "
                                                   "Green, Blue, and Luminosity Channels."));
    m_previewTargetWidget   = previewTargetWidget();
    QWhatsThis::add( m_previewTargetWidget, i18n("<p>Here you can see the image's "
                                                 "level-adjustments preview."));
    
    // -------------------------------------------------------------
    
    // Reset all parameters to the default values.
    QTimer::singleShot(0, this, SLOT(slotDefault()));
                    
    // -------------------------------------------------------------
    // Channels and scale selection slots.

    connect(m_channelCB, SIGNAL(activated(int)),
            this, SLOT(slotChannelChanged(int)));

    connect(m_scaleBG, SIGNAL(released(int)),
            this, SLOT(slotScaleChanged(int)));
            
    connect(m_previewOriginalWidget, SIGNAL(spotPositionChanged( const Digikam::DColor &, bool, const QPoint & )),
            this, SLOT(slotSpotColorChanged( const Digikam::DColor &, bool )));
            
    connect(m_previewTargetWidget, SIGNAL(spotPositionChanged( const Digikam::DColor &, bool, const QPoint & )),
            this, SLOT(slotColorSelectedFromTarget( const Digikam::DColor & )));

    connect(m_overExposureIndicatorBox, SIGNAL(toggled (bool)),
            this, SLOT(slotEffect()));      
            
    connect(m_previewOriginalWidget, SIGNAL(signalResized()),
            this, SLOT(slotEffect()));

    connect(m_previewTargetWidget, SIGNAL(signalResized()),
            this, SLOT(slotEffect()));                                                            
                        
                        
    // -------------------------------------------------------------
    // Color sliders and spinbox slots.

    connect(m_hGradientMinInput, SIGNAL(valueChanged(int)),
            this, SLOT(slotAdjustMinInputSpinBox(int)));

    connect(m_minInput, SIGNAL(valueChanged (int)),
            this, SLOT(slotAdjustSliders()));

    connect(m_gammaInput, SIGNAL(valueChanged (double)),
            this, SLOT(slotGammaInputchanged(double)));

    connect(m_hGradientMaxInput, SIGNAL(valueChanged(int)),
            this, SLOT(slotAdjustMaxInputSpinBox(int)));

    connect(m_maxInput, SIGNAL(valueChanged (int)),
            this, SLOT(slotAdjustSliders()));

    connect(m_hGradientMinOutput, SIGNAL(valueChanged(int)),
            this, SLOT(slotAdjustMinOutputSpinBox(int)));

    connect(m_minOutput, SIGNAL(valueChanged (int)),
            this, SLOT(slotAdjustSliders()));

    connect(m_hGradientMaxOutput, SIGNAL(valueChanged(int)),
            this, SLOT(slotAdjustMaxOutputSpinBox(int)));

    connect(m_maxOutput, SIGNAL(valueChanged (int)),
            this, SLOT(slotAdjustSliders()));

    // -------------------------------------------------------------
    // Bouttons slots.

    connect(m_autoButton, SIGNAL(clicked()),
            this, SLOT(slotAutoLevels()));

    connect(m_resetButton, SIGNAL(clicked()),
            this, SLOT(slotResetCurrentChannel()));
}

AdjustLevelDialog::~AdjustLevelDialog()
{
    m_histogramWidget->stopHistogramComputation();

    if (m_destinationPreviewData) 
       delete [] m_destinationPreviewData;
       
    delete m_histogramWidget;
    delete m_levelsHistogramWidget;
    delete m_levels;
}

void AdjustLevelDialog::slotSpotColorChanged(const Digikam::DColor &color, bool release)
{
    if ( m_pickBlack->isOn() )
    {
       // Black tonal levels point.
       m_levels->levelsBlackToneAdjustByColors(m_channelCB->currentItem(), color);      
       m_pickBlack->setOn(!release);
    }
    else if ( m_pickGray->isOn() )
    {
       // Gray tonal levels point.
       m_levels->levelsGrayToneAdjustByColors(m_channelCB->currentItem(), color);      
       m_pickGray->setOn(!release);
    }
    else if ( m_pickWhite->isOn() )
    {
       // White tonal levels point.
       m_levels->levelsWhiteToneAdjustByColors(m_channelCB->currentItem(), color);      
       m_pickWhite->setOn(!release);
    }
    else
       m_levelsHistogramWidget->setHistogramGuideByColor(color);

    // Refresh the current levels config.
    slotChannelChanged(m_channelCB->currentItem());
       
    slotEffect();                
}

void AdjustLevelDialog::slotColorSelectedFromTarget( const Digikam::DColor &color )
{
    m_histogramWidget->setHistogramGuideByColor(color);
}

void AdjustLevelDialog::slotGammaInputchanged(double val)
{
    blockSignals(true);
    m_levels->setLevelGammaValue(m_channelCB->currentItem(), val);
    blockSignals(false);
    slotTimer();
}

void AdjustLevelDialog::slotAdjustMinInputSpinBox(int val)
{
    blockSignals(true);

    if ( val < m_hGradientMaxInput->value() )
       val = m_hGradientMaxInput->value();

    m_minInput->setValue(m_histoSegments - val);
    m_hGradientMinInput->setValue( val );
    m_levels->setLevelLowInputValue(m_channelCB->currentItem(), m_histoSegments - val);
    blockSignals(false);
    slotTimer();
}

void AdjustLevelDialog::slotAdjustMaxInputSpinBox(int val)
{
    blockSignals(true);

    if ( val > m_hGradientMinInput->value() )
       val = m_hGradientMinInput->value();

    m_maxInput->setValue(m_histoSegments - val);
    m_hGradientMaxInput->setValue( val );
    m_levels->setLevelHighInputValue(m_channelCB->currentItem(), m_histoSegments - val);
    blockSignals(false);
    slotTimer();
}

void AdjustLevelDialog::slotAdjustMinOutputSpinBox(int val)
{
    blockSignals(true);

    if ( val < m_hGradientMaxOutput->value() )
       val = m_hGradientMaxOutput->value();

    m_minOutput->setValue(m_histoSegments - val);
    m_hGradientMinOutput->setValue( val );
    m_levels->setLevelLowOutputValue(m_channelCB->currentItem(), m_histoSegments - val);
    blockSignals(false);
    slotTimer();
}

void AdjustLevelDialog::slotAdjustMaxOutputSpinBox(int val)
{
    blockSignals(true);

    if ( val > m_hGradientMinOutput->value() )
       val = m_hGradientMinOutput->value();

    m_maxOutput->setValue(m_histoSegments - val);
    m_hGradientMaxOutput->setValue( val );
    m_levels->setLevelHighOutputValue(m_channelCB->currentItem(), m_histoSegments - val);
    blockSignals(false);
    slotTimer();
}

void AdjustLevelDialog::slotAdjustSliders()
{
    adjustSliders(m_minInput->value(), m_gammaInput->value(),
                  m_maxInput->value(), m_minOutput->value(),
                  m_maxOutput->value());
}

void AdjustLevelDialog::adjustSliders(int minIn, double gamIn, int maxIn, int minOut, int maxOut)
{
    m_hGradientMinInput->setValue(m_histoSegments - minIn);
    m_hGradientMaxInput->setValue(m_histoSegments - maxIn);
    m_gammaInput->setValue(gamIn);
    m_hGradientMinOutput->setValue(m_histoSegments - minOut);
    m_hGradientMaxOutput->setValue(m_histoSegments - maxOut);
}

void AdjustLevelDialog::slotResetCurrentChannel()
{
    m_levels->levelsChannelReset(m_channelCB->currentItem());

    // Refresh the current levels config.
    slotChannelChanged(m_channelCB->currentItem());
    m_levelsHistogramWidget->reset();

    slotEffect();
    m_histogramWidget->reset();
}

void AdjustLevelDialog::slotAutoLevels()
{
    // Calculate Auto levels.
    m_levels->levelsAuto(m_levelsHistogramWidget->m_imageHistogram);

    // Refresh the current levels config.
    slotChannelChanged(m_channelCB->currentItem());

    slotEffect();
}

void AdjustLevelDialog::slotEffect()
{
    Digikam::ImageIface* iface = m_previewTargetWidget->imageIface();
    uchar *orgData             = iface->getPreviewImage();
    int w                      = iface->previewWidth();
    int h                      = iface->previewHeight();
    bool sb                    = iface->previewSixteenBit();
    
    // Create the new empty destination image data space.
    m_histogramWidget->stopHistogramComputation();

    if (m_destinationPreviewData) 
       delete [] m_destinationPreviewData;
    
    m_destinationPreviewData = new uchar[w*h*(sb ? 8 : 4)];

    // Calculate the LUT to apply on the image.
    m_levels->levelsLutSetup(Digikam::ImageHistogram::AlphaChannel, m_overExposureIndicatorBox->isChecked());

    // Apply the lut to the image.
    m_levels->levelsLutProcess(orgData, m_destinationPreviewData, w, h);

    iface->putPreviewImage(m_destinationPreviewData);
    m_previewTargetWidget->updatePreview();

    // Update histogram.
    m_histogramWidget->updateData(m_destinationPreviewData, w, h, sb, 0, 0, 0, false);
    
    delete [] orgData;
}

void AdjustLevelDialog::slotOk()
{
    kapp->setOverrideCursor( KCursor::waitCursor() );
    Digikam::ImageIface* iface = m_previewTargetWidget->imageIface();
    uchar *orgData             = iface->getOriginalImage();
    int w                      = iface->originalWidth();
    int h                      = iface->originalHeight();
    bool sb                    = iface->originalSixteenBit();

    // Create the new empty destination image data space.
    uchar* desData = new uchar[w*h*(sb ? 8 : 4)];

    // Calculate the LUT to apply on the image.
    m_levels->levelsLutSetup(Digikam::ImageHistogram::AlphaChannel);

    // Apply the lut to the image.
    m_levels->levelsLutProcess(orgData, desData, w, h);

    iface->putOriginalImage(i18n("Adjust Level"), desData);
    kapp->restoreOverrideCursor();

    delete [] orgData;
    delete [] desData;
    accept();
}

void AdjustLevelDialog::slotChannelChanged(int channel)
{
    switch(channel)
    {
       case LuminosityChannel:
          m_histogramWidget->m_channelType = Digikam::HistogramWidget::ValueHistogram;
          m_levelsHistogramWidget->m_channelType = Digikam::HistogramWidget::ValueHistogram;
          m_hGradientMinInput->setColors( QColor( "black" ), QColor( "white" ) );
          m_hGradientMaxInput->setColors( QColor( "black" ), QColor( "white" ) );
          m_hGradientMinOutput->setColors( QColor( "black" ), QColor( "white" ) );
          m_hGradientMaxOutput->setColors( QColor( "black" ), QColor( "white" ) );
          break;
       
       case RedChannel:
          m_histogramWidget->m_channelType = Digikam::HistogramWidget::RedChannelHistogram;       
          m_levelsHistogramWidget->m_channelType = Digikam::HistogramWidget::RedChannelHistogram;
          m_hGradientMinInput->setColors( QColor( "black" ), QColor( "red" ) );
          m_hGradientMaxInput->setColors( QColor( "black" ), QColor( "red" ) );
          m_hGradientMinOutput->setColors( QColor( "black" ), QColor( "red" ) );
          m_hGradientMaxOutput->setColors( QColor( "black" ), QColor( "red" ) );
          break;

       case GreenChannel:
          m_histogramWidget->m_channelType = Digikam::HistogramWidget::GreenChannelHistogram;       
          m_levelsHistogramWidget->m_channelType = Digikam::HistogramWidget::GreenChannelHistogram;
          m_hGradientMinInput->setColors( QColor( "black" ), QColor( "green" ) );
          m_hGradientMaxInput->setColors( QColor( "black" ), QColor( "green" ) );
          m_hGradientMinOutput->setColors( QColor( "black" ), QColor( "green" ) );
          m_hGradientMaxOutput->setColors( QColor( "black" ), QColor( "green" ) );
          break;

       case BlueChannel:
          m_histogramWidget->m_channelType = Digikam::HistogramWidget::BlueChannelHistogram;
          m_levelsHistogramWidget->m_channelType = Digikam::HistogramWidget::BlueChannelHistogram;
          m_hGradientMinInput->setColors( QColor( "black" ), QColor( "blue" ) );
          m_hGradientMaxInput->setColors( QColor( "black" ), QColor( "blue" ) );
          m_hGradientMinOutput->setColors( QColor( "black" ), QColor( "blue" ) );
          m_hGradientMaxOutput->setColors( QColor( "black" ), QColor( "blue" ) );
          break;

       case AlphaChannel:
          m_histogramWidget->m_channelType = Digikam::HistogramWidget::AlphaChannelHistogram;       
          m_levelsHistogramWidget->m_channelType = Digikam::HistogramWidget::AlphaChannelHistogram;
          m_hGradientMinInput->setColors( QColor( "black" ), QColor( "white" ) );
          m_hGradientMaxInput->setColors( QColor( "black" ), QColor( "white" ) );
          m_hGradientMinOutput->setColors( QColor( "black" ), QColor( "white" ) );
          m_hGradientMaxOutput->setColors( QColor( "black" ), QColor( "white" ) );
          break;
    }

    adjustSliders(m_levels->getLevelLowInputValue(channel),
                  m_levels->getLevelGammaValue(channel),
                  m_levels->getLevelHighInputValue(channel),
                  m_levels->getLevelLowOutputValue(channel),
                  m_levels->getLevelHighOutputValue(channel));

    m_levelsHistogramWidget->repaint(false);
    m_histogramWidget->repaint(false);    
}

void AdjustLevelDialog::slotScaleChanged(int scale)
{
    m_levelsHistogramWidget->m_scaleType = scale;
    m_histogramWidget->m_scaleType = scale;
    m_histogramWidget->repaint(false);
    m_levelsHistogramWidget->repaint(false);
}

// Reset all settings.
void AdjustLevelDialog::slotDefault()
{
    for (int channel = 0 ; channel < 5 ; ++channel)
       m_levels->levelsChannelReset(channel);

    // Refresh the current levels config.
    slotChannelChanged(m_channelCB->currentItem());
    m_levelsHistogramWidget->reset();
    m_histogramWidget->reset();
    slotEffect();
}

// Load all settings.
void AdjustLevelDialog::slotUser3()
{
    KURL loadLevelsFile;

    loadLevelsFile = KFileDialog::getOpenURL(KGlobalSettings::documentPath(),
                                             QString( "*" ), this,
                                             QString( i18n("Select Gimp Levels File to Load")) );
    if( loadLevelsFile.isEmpty() )
       return;

    if ( m_levels->loadLevelsFromGimpLevelsFile( loadLevelsFile ) == false )
    {
       KMessageBox::error(this, i18n("Cannot load from the Gimp levels text file."));
       return;
    }

    // Refresh the current levels config.
    slotChannelChanged(m_channelCB->currentItem());
}

// Save all settings.
void AdjustLevelDialog::slotUser2()
{
    KURL saveLevelsFile;

    saveLevelsFile = KFileDialog::getSaveURL(KGlobalSettings::documentPath(),
                                             QString( "*" ), this,
                                             QString( i18n("Gimp Levels File to Save")) );
    if( saveLevelsFile.isEmpty() )
       return;

    if ( m_levels->saveLevelsToGimpLevelsFile( saveLevelsFile ) == false )
    {
       KMessageBox::error(this, i18n("Cannot save to the Gimp levels text file."));
       return;
    }

    // Refresh the current levels config.
    slotChannelChanged(m_channelCB->currentItem());
}

}  // NameSpace DigikamAdjustLevelsImagesPlugin

#include "adjustlevels.moc"

