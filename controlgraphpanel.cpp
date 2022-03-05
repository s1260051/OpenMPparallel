#include "controlgraphpanel.h"
#include "ui_controlgraphpanel.h"
#include <fstream>
#include <QFileDialog>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <iomanip>
#include <time.h>
#include <calibrationgraph.h>
#include <QDebug>


#define fitscount 77

using namespace CCfits;
using namespace std;

QString rawimagedirectorypath1="/Volumes/haya2tir_work3/HEAT/HEATDB/TIR_DATA_original/";


ControlGraphPanel::ControlGraphPanel(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ControlGraphPanel)
{
    ui->setupUi(this);
    this->setWindowTitle("Conversion panel");
    this->move(0,0);

    QObject::connect(ui->anotherView, SIGNAL( valueChangedY(QString) ), this, SLOT( setY(QString)) );
    QObject::connect(ui->anotherView, SIGNAL( valueChangedX(QString) ), this, SLOT( setX(QString)) );


    //初期ファイル読み込み
    QDir tmp,initialFileDirectory;
    //データパス用
    QString appPath;
    //初期設定ファイル格納用ディレクトリ
    appPath = QCoreApplication::applicationDirPath();
    initialFileDirectory.cd(appPath);

    //qDebug()<<initialFileDirectory.absolutePath();

    //initial.txtが存在する場合
    QFile iniFile(initialFileDirectory.absolutePath()+"/initial.txt");

    iniFile.open(QIODevice::ReadOnly);

    QTextStream load(&iniFile);

    load >> databPath;

    iniFile.close();

    //ui->xPositionWidget->append(databPath);


    connectDB();
}

ControlGraphPanel::~ControlGraphPanel()
{
    delete ui;
}

void ControlGraphPanel::popControlGraphPanel(){
    this->show();
    /*
    t[n]=new ControlGraphPanel;
    t[n]->show();
    n++;
*/
}

void ControlGraphPanel::on_StartConversionButton_clicked(){
    QString conversionmethod = ui->ConversionmethodcomboBox->currentText();
    if(conversionmethod=="Direct Conversion"){
        on_directConversionButton_clicked();
    }
    else if(conversionmethod=="Regression Conversion"){
        on_calibrationButton_clicked();
    }
    else if(conversionmethod=="l2b Conversion"){
        on_calibrationtoRadianceButton_clicked();
    }
    else if(conversionmethod=="l2a Conversion"){
        on_BlackbodycalibrationAllPixelButton_clicked();
    }
    else if(conversionmethod=="l2a Conversion (Directory)"){
        on_BlackbodycalibrationAllPixelButton_repeat_clicked();
    }

}

void ControlGraphPanel::on_loadFileButton_clicked()
{
    //Dialogでファイル名(パス)取得
    fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), "/Applications/HEAT_DATA", tr("Image Files (*.img *.fit *.fits *.fts *.txt *.dat)"));
    //ファイル名を描画クラスへ渡す
    if(fileName!= NULL){
        ui->fileNameBrowser->clear();
        ui->fileNameBrowser->setText(fileName.section('/',-1,-1));
        ui->anotherView->loadFileName(fileName);

        fstream ifs;
        ifs.open(&fileName.toStdString()[0],ios::in | ios::binary);
        QFileInfo fileinfo;
        fileinfo.setFile(fileName);
        QString ext = fileinfo.suffix();
        ext=ext.toLower();

        //ここ
        if (ext == "fit" || ext=="fits" || ext=="fts")//(fitファイルの検索)
        {
            auto_ptr<FITS> pInfile(0);
            try{
                pInfile.reset(new FITS(fileName.toStdString().c_str(), Read, true));
                PHDU& fitsImage=pInfile->pHDU();
                fitsImage.readAllKeys();

                String keywordstring[fitscount];
                double keyworddouble[fitscount];
                int keywordint[fitscount];
                String keywordname[fitscount];
                bool keywordbool[fitscount];
                int keywordcount;
                int i=0;
                keywordcount=0;
                keywordname[i]="SIMPLE";i++;
                keywordname[i]="BITPIX";i++;
                keywordname[i]="NAXIS";i++;
                keywordname[i]="NAXIS1";i++;
                keywordname[i]="NAXIS2";i++;
                keywordname[i]="EXTEND";i++;
                keywordname[i]="ORIGIN";i++;
                keywordname[i]="DATE";i++;
                keywordname[i]="DATE-BEG";i++;
                keywordname[i]="DATE-OBS";i++;
                keywordname[i]="DATE-END";i++;
                keywordname[i]="TELESCOP";i++;
                keywordname[i]="INSTRUME";i++;
                keywordname[i]="OBJECT";i++;
                keywordname[i]="BUNIT";i++;
                keywordname[i]="XPOSURE";i++;
                keywordname[i]="IFOV";i++;
                keywordname[i]="FILTER";i++;
                keywordname[i]="OPRGNAME";i++;
                keywordname[i]="OPRGNO";i++;
                keywordname[i]="ROI_LLX";i++;
                keywordname[i]="ROI_LLY";i++;
                keywordname[i]="ROI_URX";i++;
                keywordname[i]="ROI_URY";i++;
                keywordname[i]="DATAMAX";i++;
                keywordname[i]="DATAMIN";i++;
                keywordname[i]="MEAN";i++;
                keywordname[i]="STDEV";i++;
                keywordname[i]="MISS_VAL";i++;
                keywordname[i]="MISS_NUM";i++;
                keywordname[i]="DEAD_VAL";i++;
                keywordname[i]="DEAD_NUM";i++;
                keywordname[i]="SATU_VAL";i++;
                keywordname[i]="SATU_NUM";i++;
                keywordname[i]="IMGCMPRV";i++;
                keywordname[i]="IMGCMPAL";i++;
                keywordname[i]="IMGCMPPR";i++;
                keywordname[i]="IMG_ERR";i++;
                keywordname[i]="IMGSEQC";i++;
                keywordname[i]="IMGACCM";i++;
                keywordname[i]="BITDEPTH";i++;
                keywordname[i]="PLT_POW";i++;
                keywordname[i]="PLT_STAT";i++;
                keywordname[i]="BOL_STAT";i++;
                keywordname[i]="BOL_TRGT";i++;
                keywordname[i]="BOL_RANG";i++;
                keywordname[i]="BOL_TEMP";i++;
                keywordname[i]="PKG_TEMP";i++;
                keywordname[i]="CAS_TEMP";i++;
                keywordname[i]="SHT_TEMP";i++;
                keywordname[i]="LEN_TEMP";i++;
                keywordname[i]="BGR_VOL";i++;
                keywordname[i]="VB1_VOL";i++;
                keywordname[i]="ADOFSVOL";i++;
                keywordname[i]="HCE_TEMP";i++;
                keywordname[i]="PNL_TEMP";i++;
                keywordname[i]="AE_TEMP";i++;
                keywordname[i]="S_DISTHT";i++;
                keywordname[i]="S_DISTHE";i++;
                keywordname[i]="S_DISTHS";i++;
                keywordname[i]="S_DISTTS";i++;
                keywordname[i]="S_TGRADI";i++;
                keywordname[i]="S_APPDIA";i++;
                keywordname[i]="S_SOLLAT";i++;
                keywordname[i]="S_SOLLON";i++;
                keywordname[i]="S_SSCLAT";i++;
                keywordname[i]="S_SSCLON";i++;
                keywordname[i]="S_SSCLST";i++;
                keywordname[i]="S_SSCPX";i++;
                keywordname[i]="S_SSCPY";i++;
                keywordname[i]="S_SCXSAN";i++;
                keywordname[i]="S_SCYSAN";i++;
                keywordname[i]="S_SCZSAN";i++;
                keywordname[i]="NAIFNAME";i++;
                keywordname[i]="NAIFID";i++;
                keywordname[i]="VESION";i++;
                keywordcount=i;


                QString fitsinfoarray[fitscount];
                fitsinfoarray[0]="File Name: "+   QFileInfo(fileName).fileName();//ファイル名

                i=0;
                for(i=0;i<=keywordcount;i++){
                    try{
                        pInfile->pHDU().readKey<bool>(keywordname[i], keywordbool[i]);
                        if(typeid(keywordbool[i])==typeid(bool))
                        {
                            if(keywordbool[i]==1){
                                fitsinfoarray[1+i]=QString::fromStdString(keywordname[i])+ ":  T";
                            }
                            else fitsinfoarray[1+i]=QString::fromStdString(keywordname[i])+ ":  F";
                        }continue;
                    }
                    catch(...){};

                    try{
                        pInfile->pHDU().readKey<double>(keywordname[i], keyworddouble[i]);
                        if(typeid(keyworddouble[i])==typeid(double))
                        {
                            fitsinfoarray[1+i]=QString::fromStdString(keywordname[i])+ ": " + QString::number(keyworddouble[i]);
                        }continue;
                    }
                    catch(...){};

                    try{
                        pInfile->pHDU().readKey<int>(keywordname[i], keywordint[i]);
                        if(typeid(keywordint[i])==typeid(int))
                        {
                            fitsinfoarray[1+i]=QString::fromStdString(keywordname[i])+ ": " + QString::number(keywordint[i]);
                        }continue;
                    }
                    catch(...){};

                    try{
                        pInfile->pHDU().readKey<String>(keywordname[i], keywordstring[i]);
                        if(typeid(keywordstring[i])==typeid(String))
                        {
                            fitsinfoarray[1+i]=QString::fromStdString(keywordname[i])+ ": " + QString::fromStdString(keywordstring[i]);
                        }continue;
                    }
                    catch(...){};

                }
                emit FITSinfoSignal1(fitsinfoarray);
            }
            catch(FITS::CantCreate)
            {
                cout<<"Can't open fits image file"<<endl;
                return ;
            }
        }

    }
}

void ControlGraphPanel::DarkImage(){
    QString dark = ui->DarkcomboBox->currentText();
    if(dark=="Set Dark: 2018-06-29"){
        ui->anotherView->SetDarkImage("hyb2_tir_20180629_083003_dark.fit");
    }
    else if(dark=="Set Dark: 2018-11-09"){
        ui->anotherView->SetDarkImage("hyb2_tir_20181109_060903_dark.fit");
    }
    else if(dark=="Set Dark: No"){
        ui->anotherView->initializedarkimage();
    }
}



void ControlGraphPanel::on_substructButton_clicked(){
    //Dialogでファイル名(パス)取得
    subFileName = QFileDialog::getOpenFileName(this, tr("Open Substruct Image"), "TIR_DATA_original", tr("Image Files (*.img *.fit *.fits *.fts)"));

    //ファイル名を描画クラスへ渡す
    if(fileName!= NULL){
        ui->fileNameBrowser->append(subFileName.section('/',-1,-1));
        ui->anotherView->subtractImage(fileName, subFileName);
    }
}

/*
void ControlGraphPanel::on_CutCompressionButton_clicked(){
    //Dialogでファイル名(パス)取得
    subFileName = QFileDialog::getOpenFileName(this, tr("Open Substruct Image"), "TIR_DATA_original", tr("Image Files (*.img *.fit *.fits *.fts)"));

    //ファイル名を描画クラスへ渡す
    if(fileName!= NULL){
        ui->fileNameBrowser->append(subFileName.section('/',-1,-1));
        if(fileName.section('.',-2,-2) == "open"){
            ui->anotherView->subtractImage(fileName, subFileName);
        }else{
            ui->anotherView->subtractImage(subFileName, fileName);
        }
    }
}
*/

void ControlGraphPanel::on_substructFITSButton_clicked(){//FITS画像の引き算を行える
    //Dialogでファイル名(パス)取得
    subFileName = QFileDialog::getOpenFileName(this, tr("Open Substruct Image"), "TIR_DATA_original", tr("Image Files (*.img *.fit *.fits *.fts)"));

    //ファイル名を描画クラスへ渡す
    if(fileName!= NULL){
        ui->fileNameBrowser->append(subFileName.section('/',-1,-1));
        ui->anotherView->subtractFITSImage(subFileName);
    }
}

void ControlGraphPanel::on_outputCurrentImageButton_clicked(){

    ui->anotherView->outputCurrentImage();

}

void ControlGraphPanel::setX(QString x){
    ui->xPositionWidget->setText(x);
    emit changeX(x);
}

void ControlGraphPanel::setY(QString y){
    ui->yPositionWidget->setText(y);
    emit changeY(y);
}


void ControlGraphPanel::on_calibrationtoRadianceButton_clicked(){//輝度画像変換
    int xmin,xmax,ymin,ymax;
    int dragxstart,dragxend,dragystart,dragyend;
    int dragxend2,dragyend2;

    xmin=ui->MinxlineEdit->text().toInt();
    xmax=ui->MaxxlineEdit->text().toInt();
    ymin=ui->MinylineEdit->text().toInt();
    ymax=ui->MaxylineEdit->text().toInt();

    dragxstart=ui->anotherView->startPos_.x()-26;
    dragxend=ui->anotherView->endPos.x()-26;
    dragystart=ui->anotherView->startPos_.y()-16;
    dragyend=ui->anotherView->endPos.y()-16;
    dragxend2=ui->anotherView->endPos.x()-26;
    dragyend2=ui->anotherView->endPos.y()-16;

    if(dragxstart>dragxend){
        int tmp=0;
        tmp=dragxend;
        dragxend=dragxstart;
        dragxstart=tmp;
    }

    if(dragystart>dragyend){
        int tmp=0;
        tmp=dragyend;
        dragyend=dragystart;
        dragystart=tmp;
    }

    if(xmin==0 && dragxstart!=-26){
        xmin=dragxstart;
    }
    if(xmax==327 && dragxend!=-26){
        xmax=dragxend;
    }
    if(ymin==0 && dragystart!=-16){
        ymin=dragystart;
    }
    if(ymax==247 && dragyend!=-16){
        ymax=dragyend;
    }

    if(dragxend2==-26 && dragyend2==-16){
        xmin=0;
        xmax=327;
        ymin=0;
        ymax=247;
    }


    if(xmin<0 ||xmax>327 ||ymin<0 || ymax>247 || ymin>ymax || xmin>xmax){
        return;
    }

    else{

        //ダーク画像セット
        DarkImage();

        QString s;
        //パスの指定
        // QString initialFileDirectory = QFileDialog::getExistingDirectory(this,"Select Register Formula Folder");
        QString initialFileDirectory = QFileDialog::getExistingDirectory(this,tr("Select Register Formula Folder"),"/Applications/HEATcalibration");
        if(initialFileDirectory == ""){
            return;
        }

        QString fitdirectory= QFileDialog::getExistingDirectory(this, tr("Select the directory to save the image"),"/Applications/HEAT_DATA");

        //プログレスダイアログの設定
        QProgressDialog p;
        p.setLabelText("Calibration Process");
        p.setRange(0,0);

        ui->anotherView->initializeCalibrateImage();

        //較正式の取得
        int count=0;
        for(int j=xmin; j<xmax+1; j++){
            for(int m=ymin; m<ymax+1; m++){


                s.clear();

                //小惑星ピクセルを処理
                if(ui->anotherView->GetAsteroidPixel(m,j)==1){


                    QFile file(initialFileDirectory+"/" + QString::number(j) + "_" + QString::number(m) + ".txt");
                    //ファイルが存在する場合
                    if(file.exists()){
                        file.open(QIODevice::ReadOnly);
                        QTextStream load(&file);

                        load >> s;

                        file.close();

                    }else{ //ファイルがない場合
                        s = "0,0,0,0,0,0,0";
                    }

                    ui->anotherView->calibrateImagetoRadianceforBlackbodyAllPixel(s,j,m);//オリジナル
                }

                //プログレス表示
                p.setLabelText("Calibration Process \n" + QString::number(count) + " / " + QString::number(((xmax+1)-xmin)*((ymax+1)-ymin)) +"\n" + "x: "+QString::number(j)+"  y: "+QString::number(m));
                p.show();
                count++;
                QCoreApplication::processEvents();
                //  ui->anotherView->updateImage();//全部終わってから表示したければここを使わない

                //キャンセルボタンが押された場合は中身を空にしてリターン
                if(p.wasCanceled()){
                    ui->anotherView->updateImage(1,initialFileDirectory,fitdirectory);
                    return;
                }
            }
        }


        ui->anotherView->updateImage(1,initialFileDirectory,fitdirectory);

    }
}


void ControlGraphPanel::on_BlackbodycalibrationAllPixelButton_repeat_clicked(){//ディレクトリごと黒体輻射でキャリブレーション

    ui->anotherView->initializeFITSarray();
    int xmin,xmax,ymin,ymax;
    int dragxstart,dragxend,dragystart,dragyend;
    int dragxend2,dragyend2;

    xmin=ui->MinxlineEdit->text().toInt();
    xmax=ui->MaxxlineEdit->text().toInt();
    ymin=ui->MinylineEdit->text().toInt();
    ymax=ui->MaxylineEdit->text().toInt();

    dragxstart=ui->anotherView->startPos_.x()-26;
    dragxend=ui->anotherView->endPos.x()-26;
    dragystart=ui->anotherView->startPos_.y()-16;
    dragyend=ui->anotherView->endPos.y()-16;
    dragxend2=ui->anotherView->endPos.x()-26;
    dragyend2=ui->anotherView->endPos.y()-16;

    if(dragxstart>dragxend){
        int tmp=0;
        tmp=dragxend;
        dragxend=dragxstart;
        dragxstart=tmp;
    }

    if(dragystart>dragyend){
        int tmp=0;
        tmp=dragyend;
        dragyend=dragystart;
        dragystart=tmp;
    }

    if(xmin==0 && dragxstart!=-26){
        xmin=dragxstart;
    }
    if(xmax==327 && dragxend!=-26){
        xmax=dragxend;
    }
    if(ymin==0 && dragystart!=-16){
        ymin=dragystart;
    }
    if(ymax==247 && dragyend!=-16){
        ymax=dragyend;
    }

    if(dragxend2==-26 && dragyend2==-16){
        xmin=0;
        xmax=327;
        ymin=0;
        ymax=247;
    }
    int removecounter=0;

    if(xmin<0 ||xmax>327 ||ymin<0 || ymax>247 || ymin>ymax || xmin>xmax){
        return;
    }

    else{

        //ダーク画像セット
        DarkImage();

        QString DirName = QFileDialog::getExistingDirectory(this, tr("Open Image"), "/Applications/HEAT_DATA");
        if(DirName == ""){
            return;
        }
        QDir q_dir(DirName);
        QStringList filelist;

        QString initialFileDirectory = QFileDialog::getExistingDirectory(this,tr("Select Register Formula Folder"),"/Applications/HEATcalibration");
        if(initialFileDirectory == ""){
            return;
        }

        if(q_dir.exists()){
            filelist = q_dir.entryList();
        }
        for ( int z=2; z < filelist.size(); z++ ){//FITS以外のいらないファイルを削除
            fileName=DirName+"/"+filelist.at(z);
            // cout<<fileName.toStdString()<<endl;
            fstream ifs;
            ifs.open(&fileName.toStdString()[0],ios::in | ios::binary);
            QFileInfo fileinfo;
            fileinfo.setFile(fileName);
            QString ext = fileinfo.suffix();
            ext=ext.toLower();

            if(ext=="img" || ext=="inf"|| ext=="txt"|| ext=="png"|| ext=="plt"|| ext=="map"|| ext=="ope"|| ext=="fits"){ //|| fileName.contains("_l1.fit",Qt::CaseInsensitive)==0){
                // cout<<"DELETE: ";
                //  cout<<fileName.toStdString()<<endl;
                cout<<"DELETE: ";
                cout<<fileName.toStdString()<<endl;
                QFile::remove(fileName);
                removecounter++;
            }
        }


        QDir q_dir2(DirName);
        QStringList filelist2;
        if(q_dir2.exists()){
            filelist2 = q_dir2.entryList();
        }
        for ( int z=2; z < filelist2.size(); z++ ){//ファイルを回す
            fileName=DirName+"/"+filelist2.at(z);

            fstream ifs;
            ifs.open(&fileName.toStdString()[0],ios::in | ios::binary);
            QFileInfo fileinfo;
            fileinfo.setFile(fileName);
            QString ext = fileinfo.suffix();
            ext=ext.toLower();

            if (ext == "fit" )//(fitファイルの検索)
            {
                valarray<long> contents;
                auto_ptr<FITS> pInfile(0);
                try{
                    pInfile.reset(new FITS(fileName.toStdString().c_str(), Read, true));
                    PHDU& fitsImage=pInfile->pHDU();
                    fitsImage.read(contents);
                    fitsImage.readAllKeys();

                    double DNtmp1=0;
                    //double fitsave=0;
                    int counter=0;
                    for(int i=0; i<Height; i++){
                        for(int j=0; j<Width; j++){
                            DNtmp1=contents[counter];
                            fitsave+=DNtmp1;
                            counter++;
                        }
                    }
                    fitsave=fitsave/(Height*Width);
                }

                catch(...){};

            }


            //ごみ画像の削除
            if(fitsave>800)
            {
                cout<<"DELETE GARBAGE: ";
                cout<<fileName.toStdString()<<endl;
                QFile::remove(fileName);
                removecounter++;
            }
        }


        cout<<"----------------------------------------------------------------------------------------------------"<<endl;

        QDir q_dir3(DirName);
        QStringList filelist3;
        if(q_dir3.exists()){
            filelist3 = q_dir3.entryList();
        }

        for ( int z=2; z < filelist3.size(); z++ ){
            fileName=DirName+"/"+filelist3.at(z);

            cout<<"Now converting:  ";
            cout<<fileName.toStdString()<<endl;
            cout<<"Least file:  ";
            cout<<filelist3.size()-z<<endl;
            //ファイル名を描画クラスへ渡す
            if(fileName!= NULL){
                ui->fileNameBrowser->clear();
                ui->fileNameBrowser->setText(fileName.section('/',-1,-1));
                ui->anotherView->loadFileName(fileName);

                fstream ifs;
                ifs.open(&fileName.toStdString()[0],ios::in | ios::binary);
                QFileInfo fileinfo;
                fileinfo.setFile(fileName);
                QString ext = fileinfo.suffix();
                ext=ext.toLower();

                //ここ
                if (ext == "fit" || ext=="fits" || ext=="fts")//(fitファイルの検索)
                {
                    auto_ptr<FITS> pInfile(0);
                    try{
                        pInfile.reset(new FITS(fileName.toStdString().c_str(), Read, true));
                        PHDU& fitsImage=pInfile->pHDU();
                        fitsImage.readAllKeys();

                        String keywordstring[fitscount];
                        double keyworddouble[fitscount];
                        int keywordint[fitscount];
                        String keywordname[fitscount];
                        bool keywordbool[fitscount];
                        int keywordcount;
                        int i=0;
                        keywordcount=0;
                        keywordname[i]="SIMPLE";i++;
                        keywordname[i]="BITPIX";i++;
                        keywordname[i]="NAXIS";i++;
                        keywordname[i]="NAXIS1";i++;
                        keywordname[i]="NAXIS2";i++;
                        keywordname[i]="EXTEND";i++;
                        keywordname[i]="ORIGIN";i++;
                        keywordname[i]="DATE";i++;
                        keywordname[i]="DATE-BEG";i++;
                        keywordname[i]="DATE-OBS";i++;
                        keywordname[i]="DATE-END";i++;
                        keywordname[i]="TELESCOP";i++;
                        keywordname[i]="INSTRUME";i++;
                        keywordname[i]="OBJECT";i++;
                        keywordname[i]="BUNIT";i++;
                        keywordname[i]="XPOSURE";i++;
                        keywordname[i]="IFOV";i++;
                        keywordname[i]="FILTER";i++;
                        keywordname[i]="OPRGNAME";i++;
                        keywordname[i]="OPRGNO";i++;
                        keywordname[i]="ROI_LLX";i++;
                        keywordname[i]="ROI_LLY";i++;
                        keywordname[i]="ROI_URX";i++;
                        keywordname[i]="ROI_URY";i++;
                        keywordname[i]="DATAMAX";i++;
                        keywordname[i]="DATAMIN";i++;
                        keywordname[i]="MEAN";i++;
                        keywordname[i]="STDEV";i++;
                        keywordname[i]="MISS_VAL";i++;
                        keywordname[i]="MISS_NUM";i++;
                        keywordname[i]="DEAD_VAL";i++;
                        keywordname[i]="DEAD_NUM";i++;
                        keywordname[i]="SATU_VAL";i++;
                        keywordname[i]="SATU_NUM";i++;
                        keywordname[i]="IMGCMPRV";i++;
                        keywordname[i]="IMGCMPAL";i++;
                        keywordname[i]="IMGCMPPR";i++;
                        keywordname[i]="IMG_ERR";i++;
                        keywordname[i]="IMGSEQC";i++;
                        keywordname[i]="IMGACCM";i++;
                        keywordname[i]="BITDEPTH";i++;
                        keywordname[i]="PLT_POW";i++;
                        keywordname[i]="PLT_STAT";i++;
                        keywordname[i]="BOL_STAT";i++;
                        keywordname[i]="BOL_TRGT";i++;
                        keywordname[i]="BOL_RANG";i++;
                        keywordname[i]="BOL_TEMP";i++;
                        keywordname[i]="PKG_TEMP";i++;
                        keywordname[i]="CAS_TEMP";i++;
                        keywordname[i]="SHT_TEMP";i++;
                        keywordname[i]="LEN_TEMP";i++;
                        keywordname[i]="BGR_VOL";i++;
                        keywordname[i]="VB1_VOL";i++;
                        keywordname[i]="ADOFSVOL";i++;
                        keywordname[i]="HCE_TEMP";i++;
                        keywordname[i]="PNL_TEMP";i++;
                        keywordname[i]="AE_TEMP";i++;
                        keywordname[i]="S_DISTHT";i++;
                        keywordname[i]="S_DISTHE";i++;
                        keywordname[i]="S_DISTHS";i++;
                        keywordname[i]="S_DISTTS";i++;
                        keywordname[i]="S_TGRADI";i++;
                        keywordname[i]="S_APPDIA";i++;
                        keywordname[i]="S_SOLLAT";i++;
                        keywordname[i]="S_SOLLON";i++;
                        keywordname[i]="S_SSCLAT";i++;
                        keywordname[i]="S_SSCLON";i++;
                        keywordname[i]="S_SSCLST";i++;
                        keywordname[i]="S_SSCPX";i++;
                        keywordname[i]="S_SSCPY";i++;
                        keywordname[i]="S_SCXSAN";i++;
                        keywordname[i]="S_SCYSAN";i++;
                        keywordname[i]="S_SCZSAN";i++;
                        keywordname[i]="NAIFNAME";i++;
                        keywordname[i]="NAIFID";i++;
                        keywordname[i]="VESION";i++;
                        keywordcount=i;


                        QString fitsinfoarray[fitscount];
                        fitsinfoarray[0]="File Name: "+   QFileInfo(fileName).fileName();//ファイル名

                        i=0;
                        for(i=0;i<=keywordcount;i++){
                            try{
                                pInfile->pHDU().readKey<bool>(keywordname[i], keywordbool[i]);
                                if(typeid(keywordbool[i])==typeid(bool))
                                {
                                    if(keywordbool[i]==1){
                                        fitsinfoarray[1+i]=QString::fromStdString(keywordname[i])+ ":  T";
                                    }
                                    else fitsinfoarray[1+i]=QString::fromStdString(keywordname[i])+ ":  F";
                                }continue;
                            }
                            catch(...){};

                            try{
                                pInfile->pHDU().readKey<double>(keywordname[i], keyworddouble[i]);
                                if(typeid(keyworddouble[i])==typeid(double))
                                {
                                    fitsinfoarray[1+i]=QString::fromStdString(keywordname[i])+ ": " + QString::number(keyworddouble[i]);
                                }continue;
                            }
                            catch(...){};

                            try{
                                pInfile->pHDU().readKey<int>(keywordname[i], keywordint[i]);
                                if(typeid(keywordint[i])==typeid(int))
                                {
                                    fitsinfoarray[1+i]=QString::fromStdString(keywordname[i])+ ": " + QString::number(keywordint[i]);
                                }continue;
                            }
                            catch(...){};

                            try{
                                pInfile->pHDU().readKey<String>(keywordname[i], keywordstring[i]);
                                if(typeid(keywordstring[i])==typeid(String))
                                {
                                    fitsinfoarray[1+i]=QString::fromStdString(keywordname[i])+ ": " + QString::fromStdString(keywordstring[i]);
                                }continue;
                            }
                            catch(...){};



                        }
                        emit FITSinfoSignal1(fitsinfoarray);
                    }
                    catch(FITS::CantCreate)
                    {
                        cout<<"Can't open fits image file"<<endl;
                        return ;
                    }


                    //}
                    //}



                    QString s;
                    //パスの指定
                    //プログレスダイアログの設定
                    QProgressDialog p;
                    p.setLabelText("Calibration Process");
                    p.setRange(0,0);

                    ui->anotherView->initializeCalibrateImage();

                    //較正式の取得
                    int count=0;
                    for(int j=xmin; j<xmax+1; j++){
                        for(int m=ymin; m<ymax+1; m++){
                            /////////////////////////////////
                            s.clear();

                            //小惑星ピクセルを処理
                            if(ui->anotherView->GetAsteroidPixel(m,j)==1){


                                ////////////////////////////////

                                QFile file(initialFileDirectory+"/" + QString::number(j) + "_" + QString::number(m) + ".txt");
                                //ファイルが存在する場合
                                if(file.exists()){
                                    file.open(QIODevice::ReadOnly);
                                    QTextStream load(&file);

                                    load >> s;

                                    file.close();

                                }else{ //ファイルがない場合
                                    s = "0,0,0,0,0,0,0";
                                }

                                ui->anotherView->calibrateImageforBlackbodyAllPixel(s,j,m);//オリジナル
                            }

                            //プログレス表示
                            p.setLabelText("Calibration Process \n" + QString::number(count) + " / " + QString::number(((xmax+1)-xmin)*((ymax+1)-ymin)) +"\n" + "x: "+QString::number(j)+"  y: "+QString::number(m));
                            p.show();
                            count++;
                            QCoreApplication::processEvents();


                            //キャンセルボタンが押された場合は中身を空にしてリターン
                            if(p.wasCanceled()){
                                ui->anotherView->updateImage(0,initialFileDirectory,DirName);
                                return;
                            }
                        }
                    }


                    ui->anotherView->updateImage(0,initialFileDirectory,DirName);
                }
            }
        }
        cout<<"Conversion Finished!!!!!!!!!!!!!!!!!!!!!"<<endl;
    }
}
void ControlGraphPanel::on_BlackbodycalibrationAllPixelButton_clicked(){//1ファイルを黒体輻射でキャリブレーション

    ui->anotherView->initializeFITSarray();

    int xmin=0,xmax=0,ymin=0,ymax=0;
    int dragxstart=0,dragxend=0,dragystart=0,dragyend=0;
    int dragxend2=0,dragyend2=0;

    xmin=ui->MinxlineEdit->text().toInt();
    xmax=ui->MaxxlineEdit->text().toInt();
    ymin=ui->MinylineEdit->text().toInt();
    ymax=ui->MaxylineEdit->text().toInt();

    dragxstart=ui->anotherView->startPos_.x()-26;
    dragxend=ui->anotherView->endPos.x()-26;
    dragystart=ui->anotherView->startPos_.y()-16;
    dragyend=ui->anotherView->endPos.y()-16;
    dragxend2=ui->anotherView->endPos.x()-26;
    dragyend2=ui->anotherView->endPos.y()-16;

    if(dragxstart>dragxend){
        int tmp=0;
        tmp=dragxend;
        dragxend=dragxstart;
        dragxstart=tmp;
    }

    if(dragystart>dragyend){
        int tmp=0;
        tmp=dragyend;
        dragyend=dragystart;
        dragystart=tmp;
    }

    if(xmin==0 && dragxstart!=-26){
        xmin=dragxstart;
    }
    if(xmax==327 && dragxend!=-26){
        xmax=dragxend;
    }
    if(ymin==0 && dragystart!=-16){
        ymin=dragystart;
    }
    if(ymax==247 && dragyend!=-16){
        ymax=dragyend;
    }

    if(dragxend2==-26 && dragyend2==-16){
        xmin=0;
        xmax=327;
        ymin=0;
        ymax=247;
    }

    if(xmin<0 ||xmax>327 ||ymin<0 || ymax>247 || ymin>ymax || xmin>xmax){
        return;
    }

    else{     //ダーク画像セット
        DarkImage();



        QString s;
        //パスの指定

        QString initialFileDirectory = QFileDialog::getExistingDirectory(this,tr("Select Register Formula Folder"),"/Applications/HEATcalibration");
        if(initialFileDirectory == ""){
            return;
        }

        QString fitdirectory= QFileDialog::getExistingDirectory(this, tr("Select the directory to save the image"),"/Applications/HEAT_DATA");

        //プログレスダイアログの設定
        QProgressDialog p;
        p.setLabelText("Calibration Process");
        p.setRange(0,0);

        ui->anotherView->initializeCalibrateImage();

        //較正式の取得
        int count=0;
        for(int j=xmin; j<xmax+1; j++){
            for(int m=ymin; m<ymax+1; m++){

                s.clear();

                //小惑星ピクセルを処理
                if(ui->anotherView->GetAsteroidPixel(m,j)==1){

                    QFile file(initialFileDirectory+"/" + QString::number(j) + "_" + QString::number(m) + ".txt");
                    //ファイルが存在する場合
                    if(file.exists()){
                        file.open(QIODevice::ReadOnly);
                        QTextStream load(&file);

                        load >> s;

                        file.close();

                    }else{ //ファイルがない場合
                        s = "0,0,0,0,0,0,0";
                    }

                    //比演算を計算したい時ここを使う
                    ui->anotherView->calibrateImageforBlackbodyAllPixel(s,j,m);//オリジナル
                }

                //プログレス表示
                p.setLabelText("Calibration Process \n" + QString::number(count) + " / " + QString::number(((xmax+1)-xmin)*((ymax+1)-ymin)) +"\n" + "x: "+QString::number(j)+"  y: "+QString::number(m));
                p.show();
                count++;
                QCoreApplication::processEvents();


                //キャンセルボタンが押された場合は中身を空にしてリターン
                if(p.wasCanceled()){
                    ui->anotherView->updateImage(0,initialFileDirectory,fitdirectory);
                    return;
                }
            }
        }


        ui->anotherView->updateImage(0,initialFileDirectory,fitdirectory);

    }
}


/*

void ControlGraphPanel::on_confirmation_clicked(){//温度差
    int xmin,xmax,ymin,ymax;
    int dragxstart,dragxend,dragystart,dragyend;
    int dragxend2,dragyend2;

    xmin=ui->MinxlineEdit->text().toInt();
    xmax=ui->MaxxlineEdit->text().toInt();
    ymin=ui->MinylineEdit->text().toInt();
    ymax=ui->MaxylineEdit->text().toInt();

    dragxstart=ui->anotherView->startPos_.x()-26;
    dragxend=ui->anotherView->endPos.x()-26;
    dragystart=ui->anotherView->startPos_.y()-16;
    dragyend=ui->anotherView->endPos.y()-16;
    dragxend2=ui->anotherView->endPos.x()-26;
    dragyend2=ui->anotherView->endPos.y()-16;

    if(dragxstart>dragxend){
        int tmp=0;
        tmp=dragxend;
        dragxend=dragxstart;
        dragxstart=tmp;
    }

    if(dragystart>dragyend){
        int tmp=0;
        tmp=dragyend;
        dragyend=dragystart;
        dragystart=tmp;
    }

    if(xmin==0 && dragxstart!=-26){
        xmin=dragxstart;
    }
    if(xmax==327 && dragxend!=-26){
        xmax=dragxend;
    }
    if(ymin==0 && dragystart!=-16){
        ymin=dragystart;
    }
    if(ymax==247 && dragyend!=-16){
        ymax=dragyend;
    }

    if(dragxend2==-26 && dragyend2==-16){
        xmin=0;
        xmax=327;
        ymin=0;
        ymax=247;
    }

    if(xmin<0 ||xmax>327 ||ymin<0 || ymax>247 || ymin>ymax || xmin>xmax){
        return;
    }

    else{

        QString s;
        //パスの指定

        //QString initialFileDirectory = QFileDialog::getExistingDirectory(this,"Select Register Formula Folder");
        QString initialFileDirectory = QFileDialog::getExistingDirectory(this,tr("Select Register Formula Folder"),"/Applications/HEATcalibration");
        if(initialFileDirectory == ""){
            return;
        }


        //プログレスダイアログの設定
        QProgressDialog p;
        p.setLabelText("Calibration Process");
        p.setRange(0,0);

        ui->anotherView->initializeCalibrateImage();

        //比演算を計算したい時ここを使う
        QString subFileName1 = QFileDialog::getOpenFileName(this, tr("Open Substruct Image"), "TIR_DATA_original", tr("Image Files (*.img *.fit *.fits *.fts *.inf)"));


        //較正式の取得
        int count=0;
        for(int j=xmin; j<xmax+1; j++){
            for(int m=ymin; m<ymax+1; m++){

                s.clear();

                QFile file(initialFileDirectory+"/" + QString::number(j) + "_" + QString::number(m) + ".txt");
                //ファイルが存在する場合
                if(file.exists()){
                    file.open(QIODevice::ReadOnly);
                    QTextStream load(&file);

                    load >> s;

                    file.close();

                }else{ //ファイルがない場合
                    s = "0,0,0,0,0,0,0";
                }

                //比演算を計算したい時ここを使う
                ui->anotherView->confirmation(s,j,m,subFileName1);//オリジナル


                //プログレス表示
                p.setLabelText("Calibration Process \n" + QString::number(count) + " / " + QString::number(((xmax+1)-xmin)*((ymax+1)-ymin)) +"\n" + "x: "+QString::number(j)+"  y: "+QString::number(m));
                p.show();
                count++;
                QCoreApplication::processEvents();
                //  ui->anotherView->updateImage();//全部終わってから表示したければここを使わない

                //キャンセルボタンが押された場合は中身を空にしてリターン
                if(p.wasCanceled()){
                    ui->anotherView->updateImage(0,initialFileDirectory,NULL);
                    return;
                }
            }
        }


        ui->anotherView->updateImage(0,initialFileDirectory,NULL);
    }
}
*/


void ControlGraphPanel::on_calibrationButton_clicked(){//回帰
    int xmin,xmax,ymin,ymax;
    int dragxstart,dragxend,dragystart,dragyend;
    int dragxend2,dragyend2;

    xmin=ui->MinxlineEdit->text().toInt();
    xmax=ui->MaxxlineEdit->text().toInt();
    ymin=ui->MinylineEdit->text().toInt();
    ymax=ui->MaxylineEdit->text().toInt();

    dragxstart=ui->anotherView->startPos_.x()-26;
    dragxend=ui->anotherView->endPos.x()-26;
    dragystart=ui->anotherView->startPos_.y()-16;
    dragyend=ui->anotherView->endPos.y()-16;
    dragxend2=ui->anotherView->endPos.x()-26;
    dragyend2=ui->anotherView->endPos.y()-16;

    if(dragxstart>dragxend){
        int tmp=0;
        tmp=dragxend;
        dragxend=dragxstart;
        dragxstart=tmp;
    }

    if(dragystart>dragyend){
        int tmp=0;
        tmp=dragyend;
        dragyend=dragystart;
        dragystart=tmp;
    }

    if(xmin==0 && dragxstart!=-26){
        xmin=dragxstart;
    }
    if(xmax==327 && dragxend!=-26){
        xmax=dragxend;
    }
    if(ymin==0 && dragystart!=-16){
        ymin=dragystart;
    }
    if(ymax==247 && dragyend!=-16){
        ymax=dragyend;
    }

    if(dragxend2==-26 && dragyend2==-16){
        xmin=0;
        xmax=327;
        ymin=0;
        ymax=247;
    }

    if(xmin<0 ||xmax>327 ||ymin<0 || ymax>247 || ymin>ymax || xmin>xmax){
        return;
    }

    else{

        //ダーク画像セット
        DarkImage();

        QString s;
        //パスの指定
        QString initialFileDirectory = QFileDialog::getExistingDirectory(this,"Select Register Formula Folder");
        if(initialFileDirectory == ""){
            return;
        }

        //プログレスダイアログの設定
        QProgressDialog p;
        p.setLabelText("Calibration Process");
        p.setRange(0,0);

        ui->anotherView->initializeCalibrateImage();

        //較正式の取得
        int count=0;
        for(int j=xmin; j<xmax+1; j++){
            for(int m=ymin; m<ymax+1; m++){
                s.clear();

                //小惑星ピクセルを処理
                if(ui->anotherView->GetAsteroidPixel(m,j)==1){

                    QFile file(initialFileDirectory+"/" + QString::number(j) + "_" + QString::number(m) + ".txt");
                    //ファイルが存在する場合
                    if(file.exists()){
                        file.open(QIODevice::ReadOnly);
                        QTextStream load(&file);

                        load >> s;

                        file.close();

                    }else{ //ファイルがない場合
                        s = "0,0,0,0,0,0,0";
                    }

                    ui->anotherView->calibrateImage(s,j,m);
                }

                //プログレス表示
                p.setLabelText("Calibration Process \n" + QString::number(count) + " / " + QString::number(((xmax+1)-xmin)*((ymax+1)-ymin)) +"\n" + "x: "+QString::number(j)+"  y: "+QString::number(m));
                p.show();
                count++;
                QCoreApplication::processEvents();

                //キャンセルボタンが押された場合は中身を空にしてリターン
                if(p.wasCanceled()){
                    ui->anotherView->updateImage(0,initialFileDirectory,NULL);
                    return;
                }
            }
        }

        ui->anotherView->updateImage(0,initialFileDirectory,NULL);
    }
}


void ControlGraphPanel::on_directConversionButton_clicked()
{

    QProgressDialog p;
    p.setLabelText("Converting image");
    p.setCancelButton(0);
    p.show();
    QCoreApplication::processEvents();



    //読み込みファイル用
    imageData raw;
    imageData tmpI;
    QVector<double> tmp1;
    QVector<double> tmp2;
    int rawMAXDN=0;
    //変換に使用する画像
    QVector<imageData> images;
    //温度別の最も近い画像を選択
    QVector<imageData> selectNear;
    //選択された温度群
    QVector<double> tempItem;
    //最も近い値を格納
    double nearest=INT_MAX;
    QVector<double> diffDN(2);
    QVector<int> pointNum(2);
    QVector<double> getT(5);
    //ペア画像処理用
    QString IDm;
    int searchIDm,pairIDm,i=0;
    QString file1m,file2m;
    //軸の値を格納する配列
    QVector<double> vx,vy;
    //一時変数
    double tmp;
    QVector<double> weight(5);
    double weight2=0;
    //カウント用
    int count=0,mcount=0,count2;
    //結果
    QVector<double> resultImage;
    //途中処理
    QVector<double> select_temp;
    //距離格納
    QVector<double> dist;
    QVector<double> selectDist;
    QVector<interpolateFunction> interpolateTemp;
    //補間点群
    QVector<imageData> interpolatePoints;
    //距離の平均
    double AVE=0;
    double AVE2=0;
    int num;

    int judge=0;

    //rawfileオープン
    /*
    QString rawFileNameo = QFileDialog::getOpenFileName(this,
                                                        tr("Open Image"), QDir::homePath(), tr("Image Files (*.png *.jpg *.bmp *.img)"));
    QString rawFileNamec = QFileDialog::getOpenFileName(this,
                                                        tr("Open Image"), QDir::homePath(), tr("Image Files (*.png *.jpg *.bmp *.img)"));


    QString deepSkyFileNameo = QFileDialog::getOpenFileName(this,
                                                        tr("Open Image"), QDir::homePath(), tr("Image Files (*.png *.jpg *.bmp *.img)"));
    QString deepSkyFileNamec = QFileDialog::getOpenFileName(this,
                                                        tr("Open Image"), QDir::homePath(), tr("Image Files (*.png *.jpg *.bmp *.img)"));
                                                        */
    //QString rawFileNameo = "/Users/joker/Desktop/test_convert/TIR_3C5CBC5B_open.img";
    //  QString rawFileNamec = "/Users/joker/Desktop/test_convert/TIR_3C5CBC23_close.img";
    //  QString deepSkyFileNameo = "/Users/joker/Desktop/test_convert/TIR_3C5AA79B_open.img";
    //  QString deepSkyFileNamec = "/Users/joker/Desktop/test_convert/TIR_3C5AA75D_close.img";

    //ui->thumbnailView->subtractImage(rawFileNameo,rawFileNamec);
    //ui->thumbnailView->subtractImage(deepSkyFileNameo,deepSkyFileNamec);
    tmp1 = ui->anotherView->getImageD();

    // ui->anotherView->subtractImage(rawFileNameo,rawFileNamec);
    tmp2 = ui->anotherView->getImageD();

    count=0;

    //rawdataの画像データ格納
    for(int ii=0;ii<256;ii++){
        for(int jj=0;jj<384;jj++){
            //最大DN値保存用
            if(rawMAXDN<tmp2[count]/8.0)
                rawMAXDN=tmp2[count]/8.0;

            tmp=tmp2[count]/8.0;

            raw.image.append(tmp2[count]/8.0);
            //qDebug()<<tmp2[count]/8.0;
            count++;
        }
    }

    QString filepath,T5;

    count=0;
    filepath = QFileDialog::getOpenFileName(this,
                                            tr("Open Image"), QDir::homePath(), tr("Image Files (*.png *.jpg *.bmp *.img *.inf)"));
    //ファイル名取得
    QFile ini(filepath);

    ini.open(QIODevice::ReadOnly);

    QTextStream in(&ini);

    while(!in.atEnd()){
        in >> filepath;

        if(!QString::compare(filepath.right(6),"_T_MON")){
            in>>T5;
            getT[count]=T5.toDouble();

            count++;
        }
    }




    //rawdataのアンシラリ情報格納
    //TIR_3C5CBC5B_
    raw.bol_t   = getT[0];
    raw.pkg_t   = getT[1];
    raw.case_t  = getT[2];
    raw.shut_t  = getT[3];
    raw.lens_t  = getT[4];

    //撮像段階では不明
    raw.target_t=0;




    count=0;

    query.first();

    //変換データとして許容する範囲
    //double alpha = 5.0;
    double alpha = 2.0;
    while(1){
        //検索画像条件指定 ターゲット名 がBBであれば追加
        //検索する際に指定範囲に該当するものだけを選択
        judge=0;
        if((query.value(6).toString() == "BB" || query.value(6).toString() == "Oil_bath_BB")
                && raw.bol_t<query.value(15).toDouble() + alpha && raw.bol_t > query.value(15).toDouble() - alpha
                && raw.pkg_t<query.value(16).toDouble()  + alpha && raw.pkg_t  > query.value(16).toDouble() - alpha
                && raw.case_t<query.value(17).toDouble() + alpha && raw.case_t > query.value(17).toDouble() - alpha
                && raw.shut_t<query.value(18).toDouble() + alpha && raw.shut_t > query.value(18).toDouble() - alpha
                && raw.lens_t<query.value(19).toDouble() + alpha && raw.lens_t > query.value(19).toDouble() - alpha){

            //値格納
            tmpI.image = tmp1;
            tmpI.bol_t = query.value(15).toDouble();
            tmpI.pkg_t = query.value(16).toDouble();
            tmpI.case_t = query.value(17).toDouble();
            tmpI.shut_t = query.value(18).toDouble();
            tmpI.lens_t = query.value(19).toDouble();
            tmpI.target_t = query.value(9).toDouble();
            tmpI.distance = sqrt(pow((raw.bol_t-tmpI.bol_t),2.0)
                                 +pow((raw.pkg_t-tmpI.pkg_t),2.0)
                                 +pow((raw.case_t-tmpI.case_t),2.0)
                                 +pow((raw.shut_t-tmpI.shut_t),2.0)
                                 +pow((raw.lens_t-tmpI.lens_t),2.0));

            //distanceが大きい場合無視
            if(tmpI.distance>4.0){
                if(!query.next())break;
                continue;
            }

            //ここからペアを探す処理
            IDm = query.value(13).toString();
            //16進数の文字列を10進数に変換
            searchIDm = QString("0x"+IDm).toInt(0,16);
            //奇数処理
            if(searchIDm%2)
                pairIDm = searchIDm + 1;
            else
                pairIDm = searchIDm - 1;
            //探索側のポインタを先頭へ
            pairQuery.first();
            while(1){
                //条件合致したらループ抜ける
                if(pairQuery.value(13).toString().toInt(0,16)==pairIDm
                        &&pairQuery.value(6).toString()==query.value(6).toString()
                        && pairQuery.value(2).toString()==query.value(2).toString())
                    break;

                pairQuery.next();
            }

            databPath=rawimagedirectorypath1;
       //     qDebug()<<"file1m";
            file1m = databPath + query.value(2).toString() + "/" + query.value(0).toString();
         //   qDebug()<<file1m;
           // qDebug()<<"file2m";

            file2m = databPath + pairQuery.value(2).toString() + "/" + pairQuery.value(0).toString();
            //qDebug()<<file2m;

            if(searchIDm%2)
                ui->anotherView->subtractImage(file1m,file2m);
            else
                ui->anotherView->subtractImage(file2m,file1m);


            //計算用の画像データを配列に格納
            mcount=0;
            tmp1=ui->anotherView->getImageD();


            //これあったら全部breakするからとりあえずとばす
            //DN値が不正なデータは除く
                       if(ui->anotherView->getMinDN()<-40){
                if(!query.next())break;
                continue;
            }

            if(query.value(4).toString()!="1"){
                for(int height=0; height<256;height++){
                    for(int width=0; width<384;width++){
                        //値を格納
                        tmp1[mcount]/=8;
                        mcount++;

                    }
                }
                /*
                ui->thumbnailView->loadImage(tmp1);
                if(ui->thumbnailView->getMaxDN()<rawMAXDN-150||ui->thumbnailView->getMaxDN()>rawMAXDN+150){
                    judge=1;
                }
                */
            }

            /*
            //最大値が離れた画像を使わないようにする処理
            if(ui->thumbnailView->getMaxDN()<rawMAXDN-150||ui->thumbnailView->getMaxDN()>rawMAXDN+150||judge==1){
                if(!query.next())break;
                continue;
            }
*/


            //値格納
            tmpI.image = tmp1;
            tmpI.bol_t = query.value(15).toDouble();
            tmpI.pkg_t = query.value(16).toDouble();
            tmpI.case_t = query.value(17).toDouble();
            tmpI.shut_t = query.value(18).toDouble();
            tmpI.lens_t = query.value(19).toDouble();
            tmpI.target_t = query.value(9).toDouble();
            tmpI.distance = sqrt(pow((raw.bol_t-tmpI.bol_t),2.0)
                                 +pow((raw.pkg_t-tmpI.pkg_t),2.0)
                                 +pow((raw.case_t-tmpI.case_t),2.0)
                                 +pow((raw.shut_t-tmpI.shut_t),2.0)
                                 +pow((raw.lens_t-tmpI.lens_t),2.0));

/*
            for(int i=15;i<20;i++){
                qDebug()<<i;
                qDebug()<<query.value(i).toDouble();
            }
*/



            //最も近いdistance
            if(nearest>tmpI.distance)
                nearest=tmpI.distance;

            images.append(tmpI);

            count++;
            if(count%100==0)
                qDebug()<<count;
        }


        //終わりに達したら抜ける
        if(!query.next())break;
    }

    //
    //ここまででデータ収集完了
    //

    //使い終わったので解放
    //dnItems.detach();

    mcount=0;


    //dist近い順に並び替え
    qSort(images.begin(),images.end(),this->compDist);

    //使われる地上試験のターゲット温度を記録
    // qDebug()<<images[0].target_t;

    tempItem.append(images[0].target_t);

    for(int ii=0;ii<images.size();ii++){
        judge=-1;
        for(int k=0;k<tempItem.size();k++){
            if(tempItem[k]==images[ii].target_t)
                judge=1;
        }
        //存在しなかった場合配列に追加
        if(judge!=1){
            tempItem.append(images[ii].target_t);
        }
    }


    //異なるターゲット温度で一番近いものを集める(5番目に近いものまで集める)
    for(int jj=0;jj<tempItem.size();jj++){
        count=0;
        count2=0;
        for(int k=0;k<images.size();k++){
            if(tempItem[jj]==images[k].target_t){
                if(count<=count2){
                    interpolatePoints.append(images[k]);
                    // qDebug()<<images[k].target_t;
                    // qDebug()<<images[k].distance;
                    count++;
                    count2=0;
                }
                count2++;
            }
            if(count==5)
                break;
        }

    }

    //近い順にそーと
    //qSort(interpolatePoints.begin(),interpolatePoints.end(),this->compDist);


    //TargetT vs DN
    //emit showConvertGraphSignal(images,"DN", "Temperature","no line",images.size());

    //距離によって温度に重みをつけ各ピクセルごとに温度を変換を行う
    //検出器座標系のみの範囲を計算




    count=0;
    count2=0;
    //代入用
    interpolateFunction tmpT;

    for(int h=0; h<256;h++){
        for(int w=0; w<384;w++){
            //検出器座標系
            if(6<h && h <248 && 16<w && w<328){

                //5つの T(DN,targetT) を計算
                for(int ii=0;ii<5;ii++){
                    //初期化
                    tmpT.DN=0;
                    tmpT.targetT=0;
                    tmpT.distAve=0;
                    diffDN[0] = 100000;
                    diffDN[1] = 100000;
                    pointNum[0]=-1;
                    pointNum[1]=-1;
                    //各点のtargetTでrawを補間するための中間温度作成,distanceの平均を計算
                    for(int jj=0;jj<tempItem.size();jj++){

                        //一番近い点と2番目に近い点のDNの差を記録、またその配列の番号を記録
                        if(diffDN[0] > sqrt(pow(raw.image[count]-interpolatePoints[ii+jj*5].image[count],2.0))){
                            diffDN[1]  = diffDN[0];
                            pointNum[1]= pointNum[0];
                            diffDN[0]  = sqrt(pow(raw.image[count]-interpolatePoints[ii+jj*5].image[count],2.0));
                            pointNum[0]= ii+jj*5;
                        }
                        else if(diffDN[1]>sqrt(pow(raw.image[count]-interpolatePoints[ii+jj*5].image[count],2.0)))
                        {
                            diffDN[1] = sqrt(pow(raw.image[count]-interpolatePoints[ii+jj*5].image[count],2.0));
                            pointNum[1] = ii+jj*5;
                        }

                    }

                    //各距離合計
                    tmp = (diffDN[0]+diffDN[1]);
                    //qDebug()<<tmp<<diffDN[0]<<diffDN[1];

                    //距離に応じてターゲット温度を補間
                    tmpT.targetT = (interpolatePoints[pointNum[0]].target_t+273.15)*(1-(diffDN[0]/tmp))
                            +(interpolatePoints[pointNum[1]].target_t+273.15)*(1-(diffDN[1]/tmp));

                    //平均距離計算
                    for(int jj=0;jj<tempItem.size();jj++){
                        tmpT.distAve+=interpolatePoints[ii+jj*5].distance;
                    }
                    tmpT.distAve /= tempItem.size();

                    //qDebug()<<tmpT.DN;
                    //qDebug()<<tmpT.targetT;

                    //重複しているので解消
                    interpolateTemp.append(tmpT);
                }

                tmp=0;
                weight2=0;

                //比例配分用
                num=0;
                for(int ii=count2*5;ii<(count2+1)*5;ii++){
                    weight[num] = interpolateTemp[ii].distAve;
                    weight2 += interpolateTemp[ii].distAve;
                    num++;
                }
                qSort(weight.begin(),weight.end(),qGreater<double>());


                num=0;
                for(int ii=count2*5;ii<(count2+1)*5;ii++)
                {
                    //近いほど距離に重みをつける
                    tmp += (interpolateTemp[ii].targetT) * (weight[num]/weight2);


                    num++;
                }
                qDebug()<<tmp;
                count2++;
                resultImage.append(tmp);
            }
            else
                resultImage.append(240);

            count++;
            qDebug()<<count;
        }
    }




    //ターゲット以外の温度を統一
    //検出器座標系のみの範囲を計算
    count=0;




    for(int h=0; h<256;h++){
        for(int w=0; w<384;w++){
            //検出器座標系
            if(6<h && h <248 && 16<w && w<328){
                AVE=0;
                AVE2=0;
                count2=0;
                for(int x=-2;x<=2;x++){
                    for(int y=-2;y<=2;y++){
                        AVE += resultImage[count+x+384*y];
                        if(resultImage[count+x+384*y]>240.9||resultImage[count+x+384*y]<239.1)
                            count2++;
                    }
                }
                AVE/=25.0;

                AVE2/=5.0;
                //条件を満たさない場合値を変更
                if(AVE<240.8&&AVE>239.2&&count2<2){
                    resultImage[count] = 240;
                }
                else if(AVE2<240.8&&AVE2>239.2)
                    resultImage[count] = 240;
                //qDebug()<<"test";
            }
            else
                resultImage[count] = 240;


            count++;
        }
    }


    count=0;
    for(int h=0; h<256;h++){
        for(int w=0; w<384;w++){
            if(resultImage[count] == 240)
                resultImage[count]=150;
                //resultImage[count]=68.15;
            count++;
        }

    }

    qDebug()<<"fin";

    for(int ii=0;ii<5;ii++)
    {
        qDebug()<<interpolateTemp[ii].targetT;
    }

    //グラフ描画 targetT vs distance
    //emit emit showConvertGraphSignal(interpolateTemp,"DN",Temperature","no line",images.size());
    /*
    //ファイル保存
    QFile savem("test_convert.img");
    savem.open(QIODevice::WriteOnly);

    QTextStream outm(&savem);

    count=0;
    for(int height=0; height<256;height++){
        for(int width=0; width<384;width++){
            outm << resultImage[count]<<" ";
            //qDebug()<<count<<resultImage[count];
            count++;
        }
        outm << endl;
    }

    savem.close();
*/
    //描画
    ui->anotherView->loadImageD(resultImage);
    //ui->thumbnailView->loadImage(tmp2);


}


//データパス取得保留
void ControlGraphPanel::getDataPath(QString path){

}

//比較判定用関数
bool ControlGraphPanel::compDist(const imageData e1, const imageData e2)
{
    return e1.distance < e2.distance;
}

//データベース接続
void ControlGraphPanel::connectDB(){

    query = QSqlQuery(db);
    query.exec(QString("SELECT * FROM tirimageinfo"));

    pairQuery = QSqlQuery(db);
    pairQuery.exec(QString("SELECT * FROM tirimageinfo"));
}
