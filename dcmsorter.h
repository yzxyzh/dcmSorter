#ifndef DCMSORTER_H
#define DCMSORTER_H

#include <QMainWindow>

#include <opencv2/core/core.hpp>

#include <itkImage.h>

#include <itkImageRegionIterator.h>
#include <itkImageRegionConstIterator.h>

#include <tinyxml.h>

#include <vector>
#include <string>

#include <sstream>

namespace Ui {
class DcmSorter;
}

class DcmSorter : public QMainWindow
{
    Q_OBJECT
    
public:
    typedef itk::Image<short, 3> itkImageType;

    
    explicit DcmSorter(QWidget *parent = 0);
    ~DcmSorter();

private:
    Ui::DcmSorter *ui;

protected:
    int                   WW;//window width;
    int                   WL;//window level;
    int                   currentLayer;
    itkImageType::Pointer originalImage;
    double                blendFactor;
    void                  UpdateALL();
    cv::Mat               showImg;
    void                  ShowError(QString str);
    
    QString               dcmLocation;
    QString               mhdLocation;
    
    TiXmlDocument         doc;
    
    std::vector<std::string>   dataNames;
    
    itkImageType::Pointer originalMask;
    
    template<typename TImage>
    void DeepCopy(typename TImage::Pointer input, typename TImage::Pointer output);
    
    std::string any2Str(int& in);
    
    void                  Reset();
    
    void                  ReloadDB();
    
public slots:
    void OnOpenDCMFolder();
    void OnOpenMHDFolder();

    void OnWLChanged(int);
    void ONWWChanged(int);
    void OnLayerChanged(int);

    void OnBlenderChanged(int);
    
    void OnWriteDB();
};

template<typename TImage>
void DcmSorter::DeepCopy(typename TImage::Pointer input, typename TImage::Pointer output)
{
    output->SetOrigin(this->originalImage->GetOrigin());
    output->SetSpacing(this->originalImage->GetSpacing());
    
    output->SetRegions(input->GetLargestPossibleRegion());
    output->Allocate();
    
    itk::ImageRegionConstIterator<TImage> inputIterator(input, input->GetLargestPossibleRegion());
    itk::ImageRegionIterator<TImage> outputIterator(output, output->GetLargestPossibleRegion());
    
    while(!inputIterator.IsAtEnd())
    {
        outputIterator.Set(inputIterator.Get());
        ++inputIterator;
        ++outputIterator;
    }
}


#endif // DCMSORTER_H
