#include "dcmsorter.h"
#include "ui_dcmsorter.h"

#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImageFileReader.h>

#include <itkMetaImageIO.h>
#include <itkMinimumMaximumImageFilter.h>
#include <itkImageFileWriter.h>
#include <itkMultiplyImageFilter.h>

#include <itkThresholdImageFilter.h>

#include <QFileDialog>

#include <iostream>
#include <QMessageBox>

#include "myMacro.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <QSize>



using namespace cv;
using namespace std;

std::string DcmSorter::any2Str(int& in)
{
    std::stringstream ss;
    ss<<in;
    std::string str;
    ss>>str;
    return str;
}



void DcmSorter::Reset()
{
    this->WL = 40;
    this->WW = 350;
    this->currentLayer = 0;
    this->blendFactor = 0.5;
    ui->WLAdjuster->setSliderPosition(240);
    ui->WWAdjuster->setSliderPosition(350);
    ui->blendAdjuster->setSliderPosition(50);
    
    ui->phaseEdit->setCurrentIndex(0);
    ui->qualityEdit->setCurrentIndex(0);
    ui->tumorEdit->setCurrentIndex(0);
    ui->isLiverMaskError->setChecked(false);
    ui->descriptionEdit->setText("在此输入个性化描述");
    
}

void DcmSorter::ReloadDB()
{
    this->dataNames.clear();
    
    //doc.Clear();
    
    const string fileName = "db.xml";
    if(!doc.LoadFile(fileName.c_str()))
    {
        ShowError("CAN NOT open db.xml! Program will exit.");
        exit(1);
    }
    
    TiXmlElement* dataNode = doc.FirstChildElement();
    TiXmlElement* firstMedData = dataNode->FirstChildElement();
    
    while (NULL != firstMedData) {
        string value = firstMedData->FirstChildElement()->GetText();
        this->dataNames.push_back(value);
        firstMedData = firstMedData->NextSiblingElement();
    }
    
    cout<<"we already have : "<<dataNames.size()<<" elements in database"<<endl;
    
    for (int i=0; i<dataNames.size(); i++) {
        cout<<"path names = "<<dataNames[i]<<endl;
    }
    
}

DcmSorter::DcmSorter(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DcmSorter)
{
    ui->setupUi(this);
    //ui->dockWidget->setGeometry(0,22,78,534);
    connect(ui->WLAdjuster,SIGNAL(sliderMoved(int)),this,SLOT(OnWLChanged(int)));
    connect(ui->WWAdjuster,SIGNAL(sliderMoved(int)),this,SLOT(ONWWChanged(int)));
    connect(ui->loadDicom,SIGNAL(clicked()),this,SLOT(OnOpenDCMFolder()));
    connect(ui->layerAdjuster, SIGNAL(sliderMoved(int)), this, SLOT(OnLayerChanged(int)));
    connect(ui->blendAdjuster, SIGNAL(sliderMoved(int)), this, SLOT(OnBlenderChanged(int)));
    
    connect(ui->saveResult, SIGNAL(clicked()), this, SLOT(OnWriteDB()));
    
    
    ui->WLAdjuster->setMaximum(400);
    ui->WWAdjuster->setMaximum(600);
    ui->blendAdjuster->setMaximum(100);
    ui->layerAdjuster->setMaximum(100);
    
    //读入database
    ReloadDB();
}

void DcmSorter::OnWriteDB()
{
    bool isNewFile = (this->dataNames.end() == std::find(dataNames.begin(), dataNames.end(), this->dcmLocation.toStdString()));
    
    if(!isNewFile)
    {
        QMessageBox msgBox;
        msgBox.setText("数据库中已经有这个数据了，是否需要覆盖？");
        msgBox.addButton(QMessageBox::Ok);
        msgBox.addButton(QMessageBox::Cancel);
        
        //msgBox.setDefaultButton(QMessageBox::Yes);
        //msgBox.setDefaultButton(QMessageBox::No);
        
        //QMessageBox::question(this, "", "数据库中已经有这个数据了，是否需要覆盖？");
        
        if(QMessageBox::Ok != msgBox.exec())
        {
            return;
        }
    }
    
    //首先创建一个Node
    TiXmlElement* newNode = NULL;
    
    if(isNewFile)
    {
    int size = this->dataNames.size();
    string Name = "Med"+any2Str(size);
    newNode = new TiXmlElement(Name.c_str());
    
    }else
    {
        TiXmlElement* elem = doc.FirstChildElement()->FirstChildElement();
        
        while (elem->FirstChildElement()->GetText()!=this->dcmLocation.toStdString()) {
            elem = elem->NextSiblingElement();
        }
        
        doc.FirstChildElement()->RemoveChild(elem);
        
        int size = this->dataNames.size() - 1;
        string Name = "Med"+any2Str(size);
        newNode = new TiXmlElement(Name.c_str());
    }
    
        
    doc.FirstChildElement()->LinkEndChild(newNode);
    
    //newNode->FirstChild()->SetValue("dummy");
    //TiXmlElement* valueNode = new TiXmlElement("");
//    TiXmlText* textNode = new TiXmlText("dummy");
//    newNode->LinkEndChild(textNode);
//    
//    string str = newNode->GetText();
//    
//    cout<<str<<endl;
    //newNode->GetText();
    //newNode->FirstChild()->SetValue("dummy");
    //newNode->SetValue("dummy");

    
#if 1
    TiXmlElement* pathNode = new TiXmlElement("path");
    TiXmlText* pathTextNode = new TiXmlText(this->dcmLocation.toStdString());
    pathNode->LinkEndChild(pathTextNode);
    newNode->LinkEndChild(pathNode);
    
    TiXmlElement* segPathNode = new TiXmlElement("segPath");
    TiXmlText* segPathTextNode = new TiXmlText(this->mhdLocation.toStdString());
    segPathNode->LinkEndChild(segPathTextNode);
    newNode->LinkEndChild(segPathNode);
    
    TiXmlElement* phaseNode = new TiXmlElement("phase");
    int currIndex = ui->phaseEdit->currentIndex();
    TiXmlText* phaseTextNode = new TiXmlText(any2Str(currIndex));
    phaseNode->LinkEndChild(phaseTextNode);
    //phaseNode->SetValue(any2Str(currIndex).c_str());
    newNode->LinkEndChild(phaseNode);
    
    TiXmlElement* qualityNode = new TiXmlElement("quality");
    currIndex = ui->qualityEdit->currentIndex();
    TiXmlText* qualityTextNode = new TiXmlText(any2Str(currIndex));
    qualityNode->LinkEndChild(qualityTextNode);
    //qualityNode->SetValue(any2Str(currIndex).c_str());
    newNode->LinkEndChild(qualityNode);
    
    TiXmlElement* tumorQualityNode = new TiXmlElement("tumorQuality");
    currIndex = ui->tumorEdit->currentIndex();
    TiXmlText* tumorTextNode = new TiXmlText(any2Str(currIndex));
    tumorQualityNode->LinkEndChild(tumorTextNode);
    //tumorQualityNode->SetValue(any2Str(currIndex).c_str());
    newNode->LinkEndChild(tumorQualityNode);
    
    TiXmlElement* layerNode = new TiXmlElement("layer");
    int totalLayer = this->originalImage->GetLargestPossibleRegion().GetSize()[2];
    TiXmlText* layerTextNode = new TiXmlText(any2Str(totalLayer));
    layerNode->LinkEndChild(layerTextNode);
    //layerNode->SetValue(any2Str(totalLayer).c_str());
    newNode->LinkEndChild(layerNode);
    
    TiXmlElement* segErrorNode = new TiXmlElement("isSegError");
    if(ui->isLiverMaskError->isChecked())
    {
        TiXmlText* segErrorTextNode = new TiXmlText("TRUE");
        segErrorNode->LinkEndChild(segErrorTextNode);
        //segErrorNode->SetValue("TRUE");
    }else{
        TiXmlText* segErrorTextNode = new TiXmlText("FALSE");
        segErrorNode->LinkEndChild(segErrorTextNode);
        //segErrorNode->SetValue("FALSE");
    }
    newNode->LinkEndChild(segErrorNode);
    
    TiXmlElement* descriptionNode = new TiXmlElement("Description");
    TiXmlText* descTextNode = new TiXmlText(ui->descriptionEdit->toPlainText().toStdString());
    descriptionNode->LinkEndChild(descTextNode);
    //descriptionNode->SetValue(ui->descriptionEdit->toPlainText().toStdString().c_str());
    newNode->LinkEndChild(descriptionNode);
#endif
    //下面写文件
    doc.SaveFile();
    
    
    ReloadDB();
    
}

void DcmSorter::OnLayerChanged(int p )
{
    this->currentLayer = p;
    UpdateALL();
}

void DcmSorter::UpdateALL()
{
    //Draw raw image
    itkImageType::SizeType size = this->originalImage->GetLargestPossibleRegion().GetSize();
    itkImageType::IndexType coo;
	coo[2] = this->currentLayer;
    
    int nRows =size[1];
	int nCols =size[0];
	Mat img(nRows,nCols,CV_8UC1);
    Mat p1;
    int min,max;
	min = WL-WW/2;
	max = WL+WW/2;
    
    for (int x = 0; x < size[1];x++)
	{
		coo[1]=x;
		for (int y=0;y<size[0];y++)
		{
			coo[0]=y;
			if (originalImage->GetPixel(coo)<min)
				img.at<uchar>(x,y)=0;
			else if (originalImage->GetPixel(coo)>max)
				img.at<uchar>(x,y)=255;
			else
				img.at<uchar>(x,y)=(originalImage->GetPixel(coo)-min)*255.0/WW;
		}
	}
    
    cvtColor(img,p1,CV_GRAY2RGB);
    
    //draw mask
    Mat p2_gray ;
    Mat p2;
    itkImageType::IndexType coord;
    p2_gray.create(nRows,nCols,CV_8UC1);
	p2_gray.setTo(Scalar(0));
    coord[2] = this->currentLayer;
	for (int x=0;x<nRows;x++)
	{
		coord[1] = x;
		for (int y=0;y<nCols;y++)
		{
			coord[0] = y;
			p2_gray.at<uchar>(x,y) = (this->originalMask->GetPixel(coord)>0)*255;
		}
	}
    
    cvtColor(p2_gray,p2,CV_GRAY2RGB);
    
    //imshow("p2",p2_gray);
    
    //blend this two
    showImg = p1*blendFactor+p2*(1.0-blendFactor);

    
    //add WW & WL infromation onto image
    string wlStr;
    string wwStr;
    stringstream ss;
    ss<<WW;
    ss>>wwStr;
    stringstream ss1;
    ss1<<WL;
    ss1>>wlStr;
    
    cv::putText(showImg, "Window Level = "+wlStr, Point2i(10,30), FONT_ITALIC, 0.5, Scalar(0,0,255));
    cv::putText(showImg, "Window Width = "+wwStr, Point2i(10,60), FONT_ITALIC, 0.5, Scalar(0,0,255));
    
    //fit showImg to screen;
    int actualWidth = ui->imgLabel->size().width();
    int actualHeight = ui->imgLabel->size().height();
    
    cv::resize(showImg, showImg, cv::Size(actualHeight,actualWidth));
    
    //bind to imgLabel;
    QImage image( showImg.data, showImg.cols, showImg.rows, showImg.step, QImage::Format_RGB888 );
 	QPixmap pixmap = QPixmap::fromImage(image.rgbSwapped());
    ui->imgLabel->setPixmap(pixmap);
}

void DcmSorter::OnBlenderChanged(int p )
{
    this->blendFactor = (double) p/100.0;
    if(this->blendFactor>1.0) this->blendFactor = 1.0;
    UpdateALL();
}

DcmSorter::~DcmSorter()
{
    delete ui;
}

void DcmSorter::OnOpenDCMFolder()
{
    GUI_BEGIN
    
    this->originalImage = NULL;//release memory
    
	QString dcmFolder = QFileDialog::getExistingDirectory(this,"open dcm folder","",QFileDialog::ShowDirsOnly
                                                          | QFileDialog::DontResolveSymlinks);
    
	string fName = dcmFolder.toStdString();
    
    if(this->dataNames.end() != std::find(dataNames.begin(), dataNames.end(), fName))
    {
            QMessageBox msgBox;
            msgBox.setText("数据库中已经有这个数据了，是否继续？");
            msgBox.addButton(QMessageBox::Ok);
            msgBox.addButton(QMessageBox::Cancel);
            
            if(QMessageBox::Ok != msgBox.exec())
            {
                return;
            }
    }
    
    
    if (fName == "")
        throw std::runtime_error("read dicom files: filepath empty");
    
    
    typedef itk::GDCMImageIO       ImageIOType;
    ImageIOType::Pointer dicomIO = ImageIOType::New();
    typedef itk::GDCMSeriesFileNames NamesGeneratorType;
    NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();
    nameGenerator->SetUseSeriesDetails(true);
    nameGenerator->RecursiveOff();
    nameGenerator->SetDirectory(fName);
    
    typedef std::vector< std::string >    SeriesIdContainer;
    const SeriesIdContainer & seriesUID = nameGenerator->GetSeriesUIDs();
    
    if(0>=seriesUID.size())
        throw std::runtime_error("error in reading dicom files");
    
    std::string seriesIdentifier;
    seriesIdentifier = seriesUID.begin()->c_str();
    typedef std::vector< std::string >   FileNamesContainer;
    FileNamesContainer fileNames;
    fileNames = nameGenerator->GetFileNames( seriesIdentifier );
    if (fileNames.size()==0)
    {
        std::cerr<<"No DCM files under this folder, please check"<<std::endl;
        return;
    }
    
    typedef itk::Image<signed short,2> ImageType16;
    typedef itk::ImageFileReader<ImageType16> DICOMReader;
    DICOMReader::Pointer dcmReader=DICOMReader::New();
    dcmReader->SetFileName(fileNames[0]);
    dcmReader->SetImageIO(dicomIO);
    dcmReader->Update();
    
    double Origin[3];
    double ElementSpacing[3];
    
    int nRows=dcmReader->GetOutput()->GetLargestPossibleRegion().GetSize()[1];
    int nCols=dcmReader->GetOutput()->GetLargestPossibleRegion().GetSize()[0];
    double zSpacingOld=0;
    double zSpacingNew=0;
    zSpacingOld=dicomIO->GetOrigin(2);
    Origin[0]=dicomIO->GetOrigin(0);
    Origin[1]=dicomIO->GetOrigin(1);
    Origin[2]=dicomIO->GetOrigin(2);
    ElementSpacing[0]=dicomIO->GetSpacing(0);
    ElementSpacing[1]=dicomIO->GetSpacing(1);
    dcmReader->SetFileName(fileNames[1]);
    dcmReader->Update();
    zSpacingNew=dicomIO->GetOrigin(2);
    ElementSpacing[2]=fabs(zSpacingOld-zSpacingNew);
    
    this->originalImage = itkImageType::New();
    originalImage->SetSpacing(ElementSpacing);
    originalImage->SetOrigin(Origin);
    itkImageType::SizeType size;
    int Layer=fileNames.size();
    size[0]=nCols;size[1]=nRows;size[2]=Layer;
    originalImage->SetRegions(size);
    originalImage->Allocate();
    originalImage->FillBuffer(0);
    
    ImageType16::IndexType dcmCoord;
    itk::Image<double,3>::IndexType Coord3D;
    
    cout<<"nRows = "<<nRows<<" nCols = "<<nCols<<" layer = "<<Layer<<endl;
    
    for (int i=0;i<Layer;i++){
        dcmReader->SetFileName(fileNames[i]);
        dcmReader->Update();
        Coord3D[2]=i;
        for (int x=0;x<nRows;x++){
            dcmCoord[1]=x;
            Coord3D[1]=x;
            for (int y=0;y<nCols;y++){
                dcmCoord[0]=y;
                Coord3D[0]=y;
                originalImage->SetPixel(Coord3D,dcmReader->GetOutput()->GetPixel(dcmCoord));
            }
        }//end 2 inner fors
        
    }//end all forsw
    
    ui->layerAdjuster->setMaximum(Layer-1);
    
    dcmLocation = dcmFolder;
    
    //this->isImgSet = true;
    OnOpenMHDFolder();
    
    GUI_END
    
}

void DcmSorter::ShowError(QString str)
{
    QMessageBox msgBox;
    msgBox.setText(str);
    msgBox.exec();
}

void DcmSorter::OnOpenMHDFolder()
{
    GUI_BEGIN
    
    this->originalMask = NULL;
    
    QString filename = QFileDialog::getOpenFileName(this,"open dicom file","","Images (*.mhd)");
    
	string filenameStr = filename.toStdString();
    
    typedef itk::MetaImageIO metaImageIO;
    metaImageIO::Pointer metaIO=metaImageIO::New();
    typedef itk::ImageFileReader<itkImageType> ImageFileReader;
    ImageFileReader::Pointer reader= ImageFileReader::New();
    
    reader->SetFileName(filenameStr);
    reader->SetImageIO(metaIO);
    reader->Update();
    
    itkImageType::Pointer outputImage=reader->GetOutput();
    
    itkImageType::SizeType size=outputImage->GetLargestPossibleRegion().GetSize();
    itkImageType::SizeType originalSize = this->originalImage->GetLargestPossibleRegion().GetSize();
    
    if(size[0] != originalSize[0]
       || size[1] != originalSize[1]
       || size[2] != originalSize[2])
    {
        ShowError("mhd size mismatch! please reload");
        reader = NULL;
        OnOpenMHDFolder();
    }
    
    typedef itk::ThresholdImageFilter<itkImageType> thresholdType;
    thresholdType::Pointer thres = thresholdType::New();
    
    thres->SetInput(outputImage);
    thres->SetOutsideValue(1);
    thres->ThresholdAbove(1);
    thres->Update();
    
    
    this->originalMask = itkImageType::New();
    DeepCopy<itkImageType>(thres->GetOutput(), this->originalMask);

    mhdLocation = filename;
    
    Reset();
    
    UpdateALL();
    
    return;
    
    GUI_END
    
    OnOpenMHDFolder();
    
}

void DcmSorter::OnWLChanged(int p)
{
    this->WL = p-200;
    UpdateALL();
}

void DcmSorter::ONWWChanged(int p)
{
    this->WW = p;
    UpdateALL();
}
