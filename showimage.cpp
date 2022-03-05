#include "mainwindow.h"
#include "showimage.h"
#include "pixcelgraph.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <math.h>
#include <FITS.h>
#include <controlgraphpanel.h>
#include <calibrationgraph.h>
#include <calibration.h>

//#include <omp.h>
//#include </usr/local/Cellar/gcc/7.2.0/lib/gcc/7/gcc/x86_64-apple-darwin16.7.0/7.2.0/include/omp.h>
#include <QDir>

#define Width 384
#define Height 256

using namespace std;
using namespace CCfits;

ShowImage::ShowImage(QWidget *parent) :
    QOpenGLWidget(parent)
{
    MAX_V=0;
    MIN_V=100000;
    judge = true;
    renderunitflag = true;

    colorselect=0;

    //荒井式の温度計算に必要な変数
    h_planck = 6.62606957e-34;
    kB = 1.3806488e-23;
    c_speed = 299792458;
    c1 = 2 * M_PI * h_planck * pow(c_speed,2);
    c2 = (h_planck * c_speed) / kB;
    SIGMA = 5.670373e-8;
    loadFilter();
}

ShowImage::~ShowImage()
{

}

void ShowImage::initializeGL()
{
    glClearColor(0, 0, 0, 0);
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

}

void ShowImage::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    glLoadIdentity();
    //視野指定 フォームで作成したwidgetとサイズ合わせる必要あり
    glOrtho(-10, Width+100, Height+10, -10,-1, 1);
}

void ShowImage::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    render();
}



void ShowImage::render()
{
    //描画範囲 (DNの最大値-最小値)
    double T = MAX_V-MIN_V;

    //カラーバー描画
    if(judge==true){

        glBegin(GL_POINTS);

        for(int i=0;i<256;i++){
            for(int j=0;j<15;j++){
                glColor3dv(colorTable[255-i]);
                glVertex2d(j+395,i);
            }
        }

        glEnd();



        //カラーバー横の数値描画
        QPainter num(this);
        num.setPen(Qt::cyan);
        num.setFont(QFont("Arial", 15));

        if(renderunitflag==true){
            num.drawText(470,53,"DN");
        }
        else{
            num.drawText(470,53,"K");
        }

        num.drawText(427,24,QString::number(MAX_V));
        num.drawText(427,85,QString::number((int)(MAX_V-T/4)));
        num.drawText(427,146,QString::number((int)(MAX_V-T/2)));
        num.drawText(427,206,QString::number((int)(MAX_V-T*3/4)));
        num.drawText(427,265,QString::number(MIN_V));
        num.end();
    }

    glBegin(GL_POINTS);
    //描画
    pixelDraw(T);
    glEnd();
}

//マウスイベント関数
void ShowImage::mousePressEvent(QMouseEvent *event)
{

    //左クリックした時にx座標,y座標,DN値を取得
    if (event->buttons() & Qt::LeftButton){
        //printf("ウィンドウ座標%d,%d \n",event->x(),event->y()); //確認用
        if(event->x()>=10 && event->x() <= 394 && event->y() <= 266 && event->y()>=10){
            setValueX(QString::number(event->x()-10));
            setValueY(QString::number(event->y()-10));
            setValuePixel(QString::number(image[event->y()-10][event->x()-10]));
        }
    }

    //右クリックした時にカラーバー、数値を隠す
    if (event->buttons() & Qt::RightButton){
        //カラーバー表示切り替え
        if(judge!=true) judge = true;
        else if(judge==true) judge = false;
        this->update();
    }

    if (!rubberBand_) {
        rubberBand_ = new QRubberBand(QRubberBand::Rectangle, this);
    }
    else{
        rubberBand_->hide();
    }

    startPos_ = event->pos();
    rubberBand_->setGeometry(QRect(startPos_, QSize()));
    rubberBand_->show();
    //cout<<startPos_.x()-26<<endl;
    //cout<<startPos_.y()-16<<endl;

}

void ShowImage::mouseMoveEvent(QMouseEvent *event){
    endPos = event->pos();
    rubberBand_->setGeometry(QRect(startPos_, endPos).normalized());
    //cout<<endPos.x()<<endl;
    //cout<<endPos.y()<<endl;
}

void ShowImage::mouseReleaseEvent(QMouseEvent *event){
    Q_UNUSED(event)

    // QRubberBand ウィジェットを非表示にする。
    //rubberBand_->hide();

    // 選択した範囲を出力する。
    //  qDebug() << rubberBand_->geometry();

    //cout<<endPos.x()-26<<endl;
    //cout<<endPos.y()-16<<endl;
}

//ファイル名受け取り用関数
void ShowImage::loadFileName(QString name)
{
    //初期化
    MAX_V=0;
    MIN_V=100000;
    filename = name;
    //cout<<name.toStdString()<<endl;確認用
    //ファイル読み込み
    loadBuffer();
    //カラーバー作成
    makeColorTable();
    //反映
    this->update();

}


string ShowImage::ReplaceString(string String1,std::string String2,std::string String3)
{
    std::string::size_type  Pos( String1.find( String2 ) );

    while( Pos != std::string::npos )
    {
        String1.replace( Pos, String2.length(), String3 );
        Pos = String1.find( String2, Pos + String3.length() );
    }

    return String1;
}


//ファイル読み込み
void ShowImage::loadBuffer()
{
    fstream ifs;
    ifs.open(&filename.toStdString()[0],ios::in | ios::binary);

    QFileInfo fileinfo;
    fileinfo.setFile(filename);
    QString ext = fileinfo.suffix();
    ext=ext.toLower();


    //ここ
    if (ext == "fit" || ext=="fits" || ext=="fts")//(fitファイルの検索)
    {
        fitsflag=1;//描画関数(pixelDraw())に渡すフラグ
        valarray<double> contents;
        auto_ptr<FITS> pInfile(0);

        try{
            pInfile.reset(new FITS(filename.toStdString().c_str(), Read, true));
            PHDU& fitsImage=pInfile->pHDU();
            fitsImage.read(contents);
            fitsImage.readAllKeys();

            try{
                pInfile->pHDU().readKey<string>("ORIGIN",origin);
                pInfile->pHDU().readKey<string>("DATE",date);
                pInfile->pHDU().readKey<string>("DATE-BEG",date_beg);
                pInfile->pHDU().readKey<string>("DATE-OBS",date_obs);
                pInfile->pHDU().readKey<string>("DATE-END",date_end);
                pInfile->pHDU().readKey<string>("TELESCOP",telescop);
                pInfile->pHDU().readKey<string>("INSTRUME",instrume);
                pInfile->pHDU().readKey<string>("OBJECT",object);
                pInfile->pHDU().readKey<string>("BUNIT",bunit);
                pInfile->pHDU().readKey<double>("XPOSURE",xposure);
                pInfile->pHDU().readKey<double>("IFOV",ifov);
                pInfile->pHDU().readKey<string>("FILTER",filter);
                pInfile->pHDU().readKey<string>("OPRGNAME",oprgname);
                pInfile->pHDU().readKey<string>("OPRGNO",oprgno);
                pInfile->pHDU().readKey<double>("ROI_LLX",roi_llx);
                pInfile->pHDU().readKey<double>("ROI_LLY",roi_lly);
                pInfile->pHDU().readKey<double>("ROI_URX",roi_urx);
                pInfile->pHDU().readKey<double>("ROI_URY",roi_ury);
                pInfile->pHDU().readKey<double>("DATAMAX",datamax);
                pInfile->pHDU().readKey<double>("DATAMIN",datamin);
                pInfile->pHDU().readKey<double>("MEAN",mean);
                pInfile->pHDU().readKey<double>("STDEV",stdev);
                pInfile->pHDU().readKey<double>("MISS_VAL",miss_val);
                pInfile->pHDU().readKey<double>("MISS_NUM",miss_num);
                pInfile->pHDU().readKey<double>("DEAD_VAL",dead_val);
                pInfile->pHDU().readKey<double>("DEAD_NUM",dead_num);
                pInfile->pHDU().readKey<double>("SATU_VAL",satu_val);
                pInfile->pHDU().readKey<double>("SATU_NUM",satu_num);
                pInfile->pHDU().readKey<string>("IMGCMPRV",imgcmprv);
                pInfile->pHDU().readKey<string>("IMGCMPAL",imgcmpal);
                pInfile->pHDU().readKey<string>("IMGCMPPR",imgcmppr);
                pInfile->pHDU().readKey<double>("IMG_ERR",img_err);
                pInfile->pHDU().readKey<string>("IMGSEQC",imgseqc);
                pInfile->pHDU().readKey<double>("IMGACCM",imgaccm);
                pInfile->pHDU().readKey<double>("BITDEPTH",bitdepth);
                pInfile->pHDU().readKey<string>("PLT_POW",plt_pow);
                pInfile->pHDU().readKey<string>("PLT_STAT",plt_stat);
                pInfile->pHDU().readKey<string>("BOL_STAT",bol_stat);
                pInfile->pHDU().readKey<double>("BOL_TRGT",bol_trgt);
                pInfile->pHDU().readKey<double>("BOL_RANG",bol_rang);
                pInfile->pHDU().readKey<double>("BOL_TEMP",bol_temp);
                pInfile->pHDU().readKey<double>("PKG_TEMP",pkg_temp);
                pInfile->pHDU().readKey<double>("CAS_TEMP",cas_temp);
                pInfile->pHDU().readKey<double>("SHT_TEMP",sht_temp);
                pInfile->pHDU().readKey<double>("LEN_TEMP",len_temp);
                pInfile->pHDU().readKey<double>("BGR_VOL",bgr_vol);
                pInfile->pHDU().readKey<double>("VB1_VOL",vb1_vol);
                pInfile->pHDU().readKey<double>("ADOFSVOL",adofsvol);
                pInfile->pHDU().readKey<double>("HCE_TEMP",hce_temp);
                pInfile->pHDU().readKey<double>("PNL_TEMP",pnl_temp);
                pInfile->pHDU().readKey<double>("AE_TEMP",ae_temp);
                pInfile->pHDU().readKey<double>("S_DISTHT",s_distht);
                pInfile->pHDU().readKey<double>("S_DISTHE",s_disthe);
                pInfile->pHDU().readKey<double>("S_DISTHS",s_disths);
                pInfile->pHDU().readKey<double>("S_DISTTS",s_distts);
                pInfile->pHDU().readKey<double>("S_TGRADI",s_tgradi);
                pInfile->pHDU().readKey<double>("S_APPDIA",s_appdia);
                pInfile->pHDU().readKey<double>("S_SOLLAT",s_sollat);
                pInfile->pHDU().readKey<double>("S_SOLLON",s_sollon);
                pInfile->pHDU().readKey<double>("S_SSCLAT",s_ssclat);
                pInfile->pHDU().readKey<double>("S_SSCLON",s_ssclon);
                pInfile->pHDU().readKey<double>("S_SSCLST",s_ssclst);
                pInfile->pHDU().readKey<double>("S_SSCPX",s_sscpx);
                pInfile->pHDU().readKey<double>("S_SSCPY",s_sscpy);
                pInfile->pHDU().readKey<double>("S_SCXSAN",s_scxsan);
                pInfile->pHDU().readKey<double>("S_SCYSAN",s_scysan);
                pInfile->pHDU().readKey<double>("S_SCZSAN",s_sczsan);
                pInfile->pHDU().readKey<string>("NAIFNAME",naifname);
                pInfile->pHDU().readKey<double>("NAIFID",naifid);
                pInfile->pHDU().readKey<string>("MKNAME",mkname);
                pInfile->pHDU().readKey<double>("VERSION",version);

            }
            catch(...){};

            try{
                pInfile->pHDU().readKey<double>("PKG_TEMP",pkgt);
                pInfile->pHDU().readKey<double>("CAS_TEMP",cast);
                pInfile->pHDU().readKey<double>("SHT_TEMP",shtt);
                pInfile->pHDU().readKey<double>("LEN_TEMP",lent);
                pInfile->pHDU().readKey<double>("BITDEPTH",bitdepth);
            }
            catch(...){};

            //Fitsの平均DNを計算
            double DNtmp1=0;
            double fitsave=0;
            int counter=0;
            for(int i=0; i<Height; i++){
                for(int j=0; j<Width; j++){
                    DNtmp1=contents[counter];
                    fitsave+=DNtmp1;
                    counter++;
                }
            }
            fitsave=fitsave/(Height*Width);


            counter=0;
            double tmp1=0;

            for(int i=0; i<Height; i++){
                for(int j=0; j<Width; j++){
                    tmp1=contents[counter];
                    //Fitsの平均DNが-700以下だったら8で割る
                    if(fitsave<-700){
                        tmp1=tmp1/8;
                    }

                    if(tmp1>MAX_V)MAX_V=tmp1;
                    if(tmp1<MIN_V)MIN_V=tmp1;
                    //image[Height-1-i][j]=tmp1;
                    image[i][j]=tmp1;

                    counter++;
                    //モデルのいろは336行め付近のフラグを変更する
                }
            }



        }

        catch(FITS::CantCreate)
        {
            cout<<"Can't open fits image file"<<endl;
            return ;
        }
    }

    else if (ext == "txt" || ext=="dat"){
        QFile file1(fileinfo.filePath());
        if (!file1.open(QIODevice::ReadOnly))//読込のみでオープンできたかチェック
        {
            printf("txt dat open error\n");
            return;
        }

        QTextStream in1(&file1);
        QString str1;
        //係数読み込み
        for(int i=0;!in1.atEnd();i++){
            for (int j=0;j<4;j++){
                in1 >> str1;
                //要素x,y,z
                imagetmp[i][j] = str1.toDouble();
            }

        }

        int k=0;
        for(int i=0;i<384;i++){
            for(int j=0;j<256;j++){
                image[255-j][i]=imagetmp[k][3];//-imagetmp[k][2];
                if(image[255-j][i]>MAX_V)MAX_V=image[255-j][i];
                if(image[255-j][i]<MIN_V)MIN_V=image[255-j][i];
                k++;
            }
        }
    }


    else{
        int Count( 0 );
        int i=0;
        int j=0;
        char Data;
        unsigned short tmp;

        while( !ifs.fail() )
        {
            //縦*横*２バイト
            if( Count >= Height*Width*2)  break;  // 行数

            ifs.read( &Data,1);

            //buf[0] buf[1] → buf[1]*256+buf[0]と2biteをリトルエンディアンで変換
            //1バイト目
            //データ表示 cout <<(int)(0xff&Data)<<endl;
            tmp=0xff&Data;

            //2バイト目
            ++Count;

            ifs.read( &Data,1);


            tmp+=(0xff&Data)*256;
            if(tmp==-9999)tmp=0;
            //            tmp=tmp/8;

        //    cout<<tmp<<endl;

            if(tmp>MAX_V)MAX_V=tmp;
            if(tmp<MIN_V)MIN_V=tmp;

            image[i][j]=(double)tmp;
            j++;

            if( Count % (Width*2) == Width*2-1 ){
                i++;
                j=0;
            }
            ++Count;
        }
        ifs.close();

    }
}

//描画関数
void ShowImage::pixelDraw(double T)
{
    //256色扱うため数値範囲を256分割した範囲をdとする
    double d=T/256;

    //256階調で色を表現するため間隔dごとに数値を定義
    for(int i=0;i<256;i++){
        colorValue[i]= d*i+MIN_V;
    }

    colorValue[255]=MAX_V+1;


    for(int i=0;i<Height;i++){
        for(int j=0;j<Width;j++){
            for(int k=0;k<256;k++){
                if(image[i][j]<colorValue[k]){
                    glColor3dv(colorTable[k]);
                    glVertex2d(j,i);
                    break;
                }
            }
        }
    }
}

//カラーテーブル作成関数
void ShowImage::makeColorTable(){
    int colorArea;
    double count = 1;

    //256の配列それぞれに色を定義
    //Rainbowの Look Up Table
    if(colorselect==0 || fitsflag==1){
        colorArea = 51;
        for(int i=0;i<256;i++){
            if(count > colorArea) count=1;
            if(i==255)count=52;

            if(i<colorArea){
                colorTable[i][0] = 1-count/(colorArea+1);
                colorTable[i][1] = 0;
                colorTable[i][2] = 1;
            }
            if(colorArea<=i && i<2*colorArea){
                colorTable[i][0] = 0;
                colorTable[i][1] = count/(colorArea+1);
                colorTable[i][2] = 1;
            }
            if(2*colorArea<=i && i<3*colorArea){
                colorTable[i][0] = 0;
                colorTable[i][1] = 1;
                colorTable[i][2] = 1-count/(colorArea+1);
            }
            if(3*colorArea<=i && i<4*colorArea){
                colorTable[i][0] = count/(colorArea+1);
                colorTable[i][1] = 1;
                colorTable[i][2] = 0;
            }
            if(4*colorArea<=i && i<5*colorArea+1){
                colorTable[i][0] = 1;
                colorTable[i][1] = 1-count/(colorArea+2);
                colorTable[i][2] = 0;
            }
            count++;
        }
    }
    //Gray-ScaleのLUT
    if(colorselect==1)
        for(int i=0;i<256;i++){
            colorTable[i][0] = (double)i/255;
            colorTable[i][1] = (double)i/255;
            colorTable[i][2] = (double)i/255;
        }
    //IronのLUT
    if(colorselect==2){
        colorArea = 64;
        for(int i=0;i<256;i++){
            if(count > colorArea) count=1;

            if(i<colorArea){
                colorTable[i][0] = 0;
                colorTable[i][1] = 0;
                colorTable[i][2] = count/(colorArea+1);
            }
            if(colorArea<=i && i<2*colorArea){
                colorTable[i][0] = count/(colorArea+1);
                colorTable[i][1] = 0;
                colorTable[i][2] = 1-count/(colorArea+1);
            }
            if(2*colorArea<=i && i<3*colorArea){
                colorTable[i][0] = 1;
                colorTable[i][1] = count/(colorArea+1);
                colorTable[i][2] = 0;
            }
            if(3*colorArea<=i && i<4*colorArea){
                colorTable[i][0] = 1;
                colorTable[i][1] = 1;
                colorTable[i][2] = count/(colorArea+1);
            }

            count++;
        }
    }

}

//座標,値受け渡し用関数
void ShowImage::setValueX(QString value)
{
    int tmp = value.toInt()- 16;
    emit valueChangedX(QString::number(tmp));
}

void ShowImage::setValueY(QString value)
{
    int tmp = value.toInt() - 6;
    emit valueChangedY(QString::number(tmp));
}

void ShowImage::setValuePixel(QString value)
{
    emit valueChangedPixel(value);
}

void ShowImage::outputCurrentImage(){
    //FITS画像、ヘッダ出力用
    long naxis    =   2;
    long naxes[2] = { 384, 256 };
    std::auto_ptr<FITS> pFits(0);
    std::auto_ptr<FITS> pFits1(0);
    try
    {
        QString fitdirectory= QFileDialog::getExistingDirectory(this, tr("Select the directory to save the image"),"/Applications/HEAT_DATA");
        if(fitdirectory == ""){
            return;
        }

        const std::string fileNametemp(fitdirectory.toStdString()+"/"+"FITSoutput.fit");
        pFits.reset( new FITS(fileNametemp,DOUBLE_IMG , naxis , naxes ) );

    }
    catch (FITS::CantCreate)
    {
        return;
    }

    int nelements(1);
    nelements = std::accumulate(&naxes[0],&naxes[naxis],1,std::multiplies<double>());
    std::valarray<double> arraytemp(nelements);

    int k=0;
    for (int i = 0; i < 256; i++){
        for(int j=0;j<384;j++){

            arraytemp[k]=image[i][j];
            k++;
        }
    }

    int  fpixel(1);
    pFits->pHDU().addKey("ORIGIN",origin,"organization responsible for the data");
    pFits->pHDU().addKey("DATE",date,"date of generation of HDU in UTC");
    pFits->pHDU().addKey("DATE-BEG",date_beg,"start date of observation program (UTC)");
    pFits->pHDU().addKey("DATE-OBS",date_obs,"start date of observation (UTC)");
    pFits->pHDU().addKey("DATE-END",date_end,"end date of observation (UTC)");
    pFits->pHDU().addKey("TELESCOP",telescop,"telescope used to acquire data");
    pFits->pHDU().addKey("INSTRUME",instrume,"name of instrument");
    pFits->pHDU().addKey("OBJECT",object,"name of observed object");
    pFits->pHDU().addKey("BUNIT",bunit,"physical unit of array values");
    pFits->pHDU().writeComment("");
    pFits->pHDU().writeComment("***** TIR Common Information *****");
    pFits->pHDU().writeComment("");
    pFits->pHDU().addKey("XPOSURE",QString::number(xposure).toStdString(),"exposure time [sec]");
    pFits->pHDU().addKey("IFOV",QString::number(ifov).toStdString(),"instantaneous field of view [rad]");
    pFits->pHDU().addKey("FILTER",filter,"bandpath range of filter (um)");
    pFits->pHDU().writeComment("");
    pFits->pHDU().writeComment("***** Observation Program *****");
    pFits->pHDU().writeComment("");
    pFits->pHDU().addKey("OPRGNAME",oprgname,"observation program name");
    pFits->pHDU().addKey("OPRGNO",oprgno,"observation program number");
    pFits->pHDU().writeComment("");
    pFits->pHDU().writeComment("***** TIR Image Information *****");
    pFits->pHDU().writeComment("");
    pFits->pHDU().addKey("ROI_LLX",QString::number(roi_llx).toStdString(),"x lower-left corner pixel of image ");
    pFits->pHDU().addKey("ROI_LLY",QString::number(roi_lly).toStdString(),"y lower-left corner pixel of image");
    pFits->pHDU().addKey("ROI_URX",QString::number(roi_urx).toStdString(),"x upper-right corner pixel of image");
    pFits->pHDU().addKey("ROI_URY",QString::number(roi_ury).toStdString(),"y upper-right corner pixel of image");
    pFits->pHDU().addKey("DATAMAX",QString::number(datamax).toStdString(),"maximum data value");
    pFits->pHDU().addKey("DATAMIN",QString::number(datamin).toStdString(),"minimum data value");
    pFits->pHDU().addKey("MEAN",QString::number(mean).toStdString(),"mean value of the data");
    pFits->pHDU().addKey("STDEV",QString::number(stdev).toStdString(),"standard deviation of the data");
    pFits->pHDU().addKey("MISS_VAL",QString::number(miss_val).toStdString(),"flag value of missing pixel");
    pFits->pHDU().addKey("MISS_NUM",QString::number(miss_num).toStdString(),"number of missing pixel");
    pFits->pHDU().addKey("DEAD_VAL",QString::number(dead_val).toStdString(),"flag value of dead pixel");
    pFits->pHDU().addKey("DEAD_NUM",QString::number(dead_num).toStdString(),"number of dead pixel");
    pFits->pHDU().addKey("SATU_VAL",QString::number(satu_val).toStdString(),"flag value of saturated pixel");
    pFits->pHDU().addKey("SATU_NUM",QString::number(satu_num).toStdString(),"number of saturated pixels");
    pFits->pHDU().addKey("IMGCMPRV",imgcmprv,"compression rev.: RAW_DAT/LOSSLESS/LOSSY");
    pFits->pHDU().addKey("IMGCMPAL",imgcmpal,"compression alg.: JPEG2000/STAR_PIXEL");
    pFits->pHDU().addKey("IMGCMPPR",imgcmppr,"compression parameter");
    pFits->pHDU().addKey("IMG_ERR",QString::number(img_err).toStdString(),"onboard image proc. return status");
    pFits->pHDU().writeComment("");
    pFits->pHDU().writeComment("***** TIR Telemetry *****");
    pFits->pHDU().writeComment("");
    pFits->pHDU().addKey("IMGSEQC",imgseqc,"image sequence counter");
    pFits->pHDU().addKey("IMGACCM",QString::number(imgaccm).toStdString(),"number of accumulated images");
    pFits->pHDU().addKey("BITDEPTH",QString::number(bitdepth).toStdString(),"image bit depth");
    pFits->pHDU().addKey("PLT_POW",plt_pow,"peltier ON/OFF");
    pFits->pHDU().addKey("PLT_STAT",plt_stat,"peltier status");
    pFits->pHDU().addKey("BOL_STAT",bol_stat,"bolometer status");
    pFits->pHDU().addKey("BOL_TRGT",QString::number(bol_trgt).toStdString(),"bolometer calibration target");
    pFits->pHDU().addKey("BOL_RANG",QString::number(bol_rang).toStdString(),"bolometer calibration range");
    pFits->pHDU().addKey("BOL_TEMP",QString::number(bol_temp).toStdString(),"bolometer temperature [degC]");
    pFits->pHDU().addKey("PKG_TEMP",QString::number(pkg_temp).toStdString(),"package temperature [degC]");
    pFits->pHDU().addKey("CAS_TEMP",QString::number(cas_temp).toStdString(),"case temperature [degC]");
    pFits->pHDU().addKey("SHT_TEMP",QString::number(sht_temp).toStdString(),"shutter temperature [degC]");
    pFits->pHDU().addKey("LEN_TEMP",QString::number(len_temp).toStdString(),"lens temperature [degC]");
    pFits->pHDU().addKey("BGR_VOL",QString::number(bgr_vol).toStdString(),"BGR voltage [V]");
    pFits->pHDU().addKey("VB1_VOL",QString::number(vb1_vol).toStdString(),"VB1 voltage [V]");
    pFits->pHDU().addKey("ADOFSVOL",QString::number(adofsvol).toStdString(),"A/D_OFS voltage [V]");
    pFits->pHDU().addKey("HCE_TEMP",QString::number(hce_temp).toStdString(),"HCE TIR sensor temperature [degC]");
    pFits->pHDU().addKey("PNL_TEMP",QString::number(pnl_temp).toStdString(),"HCE TIR sensor panel temperature [degC]");
    pFits->pHDU().addKey("AE_TEMP",QString::number(ae_temp).toStdString(),"HCE TIR analog electronics temperature [degC]");
    pFits->pHDU().writeComment("");
    pFits->pHDU().writeComment("***** Observation Information by SPICE kernel *****");
    pFits->pHDU().writeComment("");
    pFits->pHDU().addKey("S_DISTHT",QString::number(s_distht).toStdString(),"distance between HYB2 and the target [km]");
    pFits->pHDU().addKey("S_DISTHE",QString::number(s_disthe).toStdString(),"distance between HYB2 and Earth [km]");
    pFits->pHDU().addKey("S_DISTHS",QString::number(s_disths).toStdString(),"distance between HYB2 and Sun [km]");
    pFits->pHDU().addKey("S_DISTTS",QString::number(s_distts).toStdString(),"distance between the target and Sun [km]");
    pFits->pHDU().addKey("S_TGRADI",QString::number(s_tgradi).toStdString(),"the target radius at the equator [km]");
    pFits->pHDU().addKey("S_APPDIA",QString::number(s_appdia).toStdString(),"apparent diameter of the target [deg]");
    pFits->pHDU().addKey("S_SOLLAT",QString::number(s_sollat).toStdString(),"sub solar latitude [deg] of the target");
    pFits->pHDU().addKey("S_SOLLON",QString::number(s_sollon).toStdString(),"sub solar longitude [deg] of the target");
    pFits->pHDU().addKey("S_SSCLAT",QString::number(s_ssclat).toStdString(),"sub S/C latitude [deg] of the target");
    pFits->pHDU().addKey("S_SSCLON",QString::number(s_ssclon).toStdString(),"sub S/C longitude [deg] of the target");
    pFits->pHDU().addKey("S_SSCLST",QString::number(s_ssclst).toStdString(),"sub S/C local solar time [h] of the target");
    pFits->pHDU().addKey("S_SSCPX",QString::number(s_sscpx).toStdString(),"sub S/C position on Image Array (axis1)");
    pFits->pHDU().addKey("S_SSCPY",QString::number(s_sscpy).toStdString(),"sub S/C position on Image Array (axis2)");
    pFits->pHDU().addKey("S_SCXSAN",QString::number(s_scxsan).toStdString(),"angle of S/C X axis and Sun direction [deg]");
    pFits->pHDU().addKey("S_SCYSAN",QString::number(s_scysan).toStdString(),"angle of S/C Y axis and Sun direction [deg]");
    pFits->pHDU().addKey("S_SCZSAN",QString::number(s_sczsan).toStdString(),"angle of S/C Z axis and Sun directino [deg]");
    pFits->pHDU().writeComment("");
    pFits->pHDU().writeComment("***** SPICE Kernels *****");
    pFits->pHDU().writeComment("");
    pFits->pHDU().addKey("NAIFNAME",naifname,"SPICE instrument name");
    pFits->pHDU().addKey("NAIFID",QString::number(naifid).toStdString(),"SPICE instrument ID");
    pFits->pHDU().addKey("MKNAME",mkname,"SPICE Meta kernel name");
    pFits->pHDU().writeComment("");
    pFits->pHDU().writeComment("***** Version ***** ");
    pFits->pHDU().writeComment("");
    pFits->pHDU().addKey("VERSION",QString::number(version).toStdString(),"version of the HDU");

    pFits->pHDU().write(fpixel,nelements,arraytemp);

}


//パラメータ変更用関数
void ShowImage::changeParameter(double min, double max,int x)
{
    if(min != max){
        MAX_V = max;
        MIN_V = min;
    }
    colorselect = x;
    makeColorTable();
    this->update();
}

//ただ画像の引き算を行う(FITS)
void ShowImage::subtractFITSImage(QString file1){

    //close-open
    //S=open SS=close
    //初期化
    MAX_V=-100000;
    MIN_V=100000;
    //ファイル読み込み


    cout<<"file1"<<endl;
    cout<<file1.toStdString()<<endl;

    QFileInfo fileinfo;
    fileinfo.setFile(file1);
    QString ext = fileinfo.suffix();
    ext=ext.toLower();


    //ここ
    if (ext == "fit" || ext=="fits" || ext=="fts")//(fitファイルの検索)
    {
        //fitsflag=1;//描画関数(pixelDraw())に渡すフラグ
        valarray<long> contents;
        auto_ptr<FITS> pInfile(0);
        try{
            pInfile.reset(new FITS(file1.toStdString().c_str(), Read, true));
            PHDU& fitsImage=pInfile->pHDU();
            fitsImage.read(contents);
            fitsImage.readAllKeys();

            try{
                pInfile->pHDU().readKey<string>("ORIGIN",origin);
                pInfile->pHDU().readKey<string>("DATE",date);
                pInfile->pHDU().readKey<string>("DATE-BEG",date_beg);
                pInfile->pHDU().readKey<string>("DATE-OBS",date_obs);
                pInfile->pHDU().readKey<string>("DATE-END",date_end);
                pInfile->pHDU().readKey<string>("TELESCOP",telescop);
                pInfile->pHDU().readKey<string>("INSTRUME",instrume);
                pInfile->pHDU().readKey<string>("OBJECT",object);
                pInfile->pHDU().readKey<string>("BUNIT",bunit);
                pInfile->pHDU().readKey<double>("XPOSURE",xposure);
                pInfile->pHDU().readKey<double>("IFOV",ifov);
                pInfile->pHDU().readKey<string>("FILTER",filter);
                pInfile->pHDU().readKey<string>("OPRGNAME",oprgname);
                pInfile->pHDU().readKey<string>("OPRGNO",oprgno);
                pInfile->pHDU().readKey<double>("ROI_LLX",roi_llx);
                pInfile->pHDU().readKey<double>("ROI_LLY",roi_lly);
                pInfile->pHDU().readKey<double>("ROI_URX",roi_urx);
                pInfile->pHDU().readKey<double>("ROI_URY",roi_ury);
                pInfile->pHDU().readKey<double>("DATAMAX",datamax);
                pInfile->pHDU().readKey<double>("DATAMIN",datamin);
                pInfile->pHDU().readKey<double>("MEAN",mean);
                pInfile->pHDU().readKey<double>("STDEV",stdev);
                pInfile->pHDU().readKey<double>("MISS_VAL",miss_val);
                pInfile->pHDU().readKey<double>("MISS_NUM",miss_num);
                pInfile->pHDU().readKey<double>("DEAD_VAL",dead_val);
                pInfile->pHDU().readKey<double>("DEAD_NUM",dead_num);
                pInfile->pHDU().readKey<double>("SATU_VAL",satu_val);
                pInfile->pHDU().readKey<double>("SATU_NUM",satu_num);
                pInfile->pHDU().readKey<string>("IMGCMPRV",imgcmprv);
                pInfile->pHDU().readKey<string>("IMGCMPAL",imgcmpal);
                pInfile->pHDU().readKey<string>("IMGCMPPR",imgcmppr);
                pInfile->pHDU().readKey<double>("IMG_ERR",img_err);
                pInfile->pHDU().readKey<string>("IMGSEQC",imgseqc);
                pInfile->pHDU().readKey<double>("IMGACCM",imgaccm);
                pInfile->pHDU().readKey<double>("BITDEPTH",bitdepth);
                pInfile->pHDU().readKey<string>("PLT_POW",plt_pow);
                pInfile->pHDU().readKey<string>("PLT_STAT",plt_stat);
                pInfile->pHDU().readKey<string>("BOL_STAT",bol_stat);
                pInfile->pHDU().readKey<double>("BOL_TRGT",bol_trgt);
                pInfile->pHDU().readKey<double>("BOL_RANG",bol_rang);
                pInfile->pHDU().readKey<double>("BOL_TEMP",bol_temp);
                pInfile->pHDU().readKey<double>("PKG_TEMP",pkg_temp);
                pInfile->pHDU().readKey<double>("CAS_TEMP",cas_temp);
                pInfile->pHDU().readKey<double>("SHT_TEMP",sht_temp);
                pInfile->pHDU().readKey<double>("LEN_TEMP",len_temp);
                pInfile->pHDU().readKey<double>("BGR_VOL",bgr_vol);
                pInfile->pHDU().readKey<double>("VB1_VOL",vb1_vol);
                pInfile->pHDU().readKey<double>("ADOFSVOL",adofsvol);
                pInfile->pHDU().readKey<double>("HCE_TEMP",hce_temp);
                pInfile->pHDU().readKey<double>("PNL_TEMP",pnl_temp);
                pInfile->pHDU().readKey<double>("AE_TEMP",ae_temp);
                pInfile->pHDU().readKey<double>("S_DISTHT",s_distht);
                pInfile->pHDU().readKey<double>("S_DISTHE",s_disthe);
                pInfile->pHDU().readKey<double>("S_DISTHS",s_disths);
                pInfile->pHDU().readKey<double>("S_DISTTS",s_distts);
                pInfile->pHDU().readKey<double>("S_TGRADI",s_tgradi);
                pInfile->pHDU().readKey<double>("S_APPDIA",s_appdia);
                pInfile->pHDU().readKey<double>("S_SOLLAT",s_sollat);
                pInfile->pHDU().readKey<double>("S_SOLLON",s_sollon);
                pInfile->pHDU().readKey<double>("S_SSCLAT",s_ssclat);
                pInfile->pHDU().readKey<double>("S_SSCLON",s_ssclon);
                pInfile->pHDU().readKey<double>("S_SSCLST",s_ssclst);
                pInfile->pHDU().readKey<double>("S_SSCPX",s_sscpx);
                pInfile->pHDU().readKey<double>("S_SSCPY",s_sscpy);
                pInfile->pHDU().readKey<double>("S_SCXSAN",s_scxsan);
                pInfile->pHDU().readKey<double>("S_SCYSAN",s_scysan);
                pInfile->pHDU().readKey<double>("S_SCZSAN",s_sczsan);
                pInfile->pHDU().readKey<string>("NAIFNAME",naifname);
                pInfile->pHDU().readKey<double>("NAIFID",naifid);
                pInfile->pHDU().readKey<string>("MKNAME",mkname);
                pInfile->pHDU().readKey<double>("VERSION",version);

            }
            catch(...){};

            try{
                pInfile->pHDU().readKey<double>("PKG_TEMP",pkgt);
                pInfile->pHDU().readKey<double>("CAS_TEMP",cast);
                pInfile->pHDU().readKey<double>("SHT_TEMP",shtt);
                pInfile->pHDU().readKey<double>("LEN_TEMP",lent);
                pInfile->pHDU().readKey<double>("BITDEPTH",bitdepth);
            }
            catch(...){};

            int counter=0;
            double tmp1=0;

            for(int i=0; i<Height; i++){
                for(int j=0; j<Width; j++){

                    tmp1=contents[counter];

                    //  tmp1=tmp1/8;

                    if(image[i][j]-tmp1>MAX_V)MAX_V=image[i][j]-tmp1;
                    if(image[i][j]-tmp1<MIN_V)MIN_V=image[i][j]-tmp1;
                    image[i][j]-=tmp1;

/*
                    cout<<"x ";
                    cout<<j-16;
                    cout<<"  y ";
                    cout<<i-6;
                    cout<<"  ";
  */
               //   cout<<image[i][j]<<endl;
                    counter++;
                    //モデルのいろは336行め付近のフラグを変更する
                }
            }
        }

        catch(FITS::CantCreate)
        {
            cout<<"Can't open fits image file"<<endl;
            return ;
        }
    }


    //カラーバー作成
    makeColorTable();
    //反映
    this->update();

}







//open closeの差を見るとき
void ShowImage::subtractImage(QString file1, QString file2){//}, QString filename){

    //open-close
    //S=open SS=close
    //初期化
    MAX_V=-100000;
    MIN_V=100000;
    //ファイル読み込み


    if (file1.contains("close", Qt::CaseInsensitive)){//file1をopen,file2をcloseにする処理
        QString tmpo;
        tmpo=file1;
        file1=file2;
        file2=tmpo;
        //    cout<<"includingclose"<<endl;
    }

    cout<<"file1"<<endl;
    cout<<file1.toStdString()<<endl;
    cout<<"file2"<<endl;
    cout<<file2.toStdString()<<endl;


    fstream ifs1;//open
    fstream ifs2;//close

    ifs1.open(&file1.toStdString()[0],ios::in | ios::binary);
    ifs2.open(&file2.toStdString()[0],ios::in | ios::binary);

    int Count( 0 );
    int i=0;
    int j=0;
    int jjj=0;
    char Data;
    double tmp;


    while(1){
        while( !ifs1.fail() )//open
        {
            //縦*横*２バイト
            if( Count >= Height*Width*2)  break;  // 行数

            ifs1.read( &Data,1);

            //buf[0] buf[1] → buf[1]*256+buf[0]と2biteをリトルエンディアンで変換
            //1バイト目
            //データ表示 cout <<(int)(0xff&Data)<<endl;

            tmp=0xff&Data;

            //2バイト目
            ++Count;

            ifs1.read( &Data,1);
            //cout <<(int)(0xff&Data)<<endl;

            tmp+=(0xff&Data)*256;
            //        tmp=tmp/8;
            image[i][j]=tmp;
            //  cout<<"f1tmp"<<endl;
            //cout<<tmp<<endl;



            j++;

            if( Count % (Width*2) == Width*2-1 ){
                i++;
                j=0;
            }

            ++Count;
        }

        ifs1.close();

        Count = 0;
        i=0;
        j=0;

        while( !ifs2.fail() )//close
        {
            //縦*横*２バイト
            if( Count >= Height*Width*2)  break;  // 行数

            ifs2.read( &Data,1);

            //buf[0] buf[1] → buf[1]*256+buf[0]と2biteをリトルエンディアンで変換
            //1バイト目
            //データ表示 cout <<(int)(0xff&Data)<<endl;

            tmp=0xff&Data;

            //2バイト目
            ++Count;

            ifs2.read( &Data,1);
            //cout <<(int)(0xff&Data)<<endl;

            tmp+=(0xff&Data)*256;

            //ここからclose画像を引く
            //tmp=tmp/8;
            image[i][j] -= tmp;


            //  cout<<"image"<<endl;
            //cout<<tmp<<endl;


            //********************************************************************//
            //マスクピクセル判定用
            /*
            //閾値設定
            if(image[i][j]> xxxx){
                int tcount = 0;
                int max1 = 0;
                int max2 = 0;
                int u1 = 0;
                int u2 = 0;
                int h =0;
                int p =0;
                int i=0;
                QString string1;
                QString string2;
                QString string3;

                for (int x = 0; x < 256; x += 32)
                {
                    max1 = x;
                    max1 = max1 + 32;
                    u1 = max1 - 1;

                    for (int y = 0; y < 384; y += 32)
                    {
                        tcount++;
                        max2 = y;
                        max2 = max2 + 32;
                        u2 = max2 - 1;
                        if (tcount < 10)
                        {
                            string1 = "UPDATE pix0" + QString::number(tcount);
                            //  printf("insert into pix0%d (img_file,x,y,pixel) values", tcount);
                        }
                        else
                        {
                            //printf("insert into pix%d (img_file,x,y,pixel) values", tcount);
                            string1 = "UPDATE pix" + QString::number(tcount);
                        }
                        for (h = x; h < max1; h++)
                        {
                            for (p = y; p < max2; p++)
                            {
                          /*      if (i == u1 && p == u2 && image[h][p]<閾値設定 )
                                {
                                    //  cout << "('" << img_file << "'," << p << "," << h << "," << pix[p][h] << ");" << endl;
                                    string2 = " SET mask=0 WHERE target_name=\""+filename+"\" AND x="+h+" AND y="+p+";";
                                }
                                else
                                {
                                    // cout << "('" << img_file << "'," << p << "," << h << "," << pix[p][h] << ");" << endl;
                                    string2 = " SET mask=0 WHERE target_name=\""+filename+"\" AND x="+i+" AND y="+j+";";
                                }

                            }
                        }
                    }
                }

                string3= string1+string2;
                cout<<string3.toStdString()<<endl;
            }
*/


            //********************************************************************//

            if(image[i][j]>MAX_V)MAX_V=image[i][j];
            if(image[i][j]<MIN_V)MIN_V=image[i][j];



            j++;

            if( Count % (Width*2) == Width*2-1 ){
                i++;
                jjj=j;
                j=0;
            }

            ++Count;
        }

        ifs2.close();


        /*


        //中央の値がオプティカルブラック領域よりも高いことを確認
        //低い場合は逆にして計算
        double AveOB=0,AveCenter=0;
        int countB=0,countC=0;
        for(int x=-2;x<=2;x++){
            for(int y=-2;y<=2;y++){
                //異常値判定
                if(image[8+y][8+x]<image[8][8]+ 10
                        && image[8+y][8+x]>image[8][8]- 10){
                    AveOB += image[8+y][8+x];
                    //異常値を除いた積算回数
                    countB++;
                }
            }
        }
        AveOB/=(double)countB;

        for(int x=-10;x<=10;x++){
            for(int y=-10;y<=10;y++){
                //異常値判定
                if(image[128+y][163+x]<image[128][163]+10
                        && image[128+y][163+x]>image[128][163]-10){
                    AveCenter += image[128+y][163+x];
                    //異常値を除いた積算回数
                    countC++;
                }
            }
        }
        AveCenter/=(double)countC;

        if(AveCenter < AveOB){
            ifs1.open(&file2.toStdString()[0],ios::in | ios::binary);
            ifs2.open(&file1.toStdString()[0],ios::in | ios::binary);
            i=0;
            j=0;
            Count=0;
        }



        //else break;
*/
        break;
    }



    if(MAX_V>1000){
        MAX_V=MAX_V/8;
        MIN_V=MIN_V/8;

        for(int xx=0;xx<i;xx++){
            for(int yy=0;yy<jjj;yy++){
                image[xx][yy]=image[xx][yy]/8;
                //MIN_V=MIN_V/8;
            }
        }
    }


    //カラーバー作成
    makeColorTable();
    //反映
    this->update();

}


//DBウィンドウのopen closeの差をFITS形式で出力
void ShowImage::OutputFITSDBsubtractImage(QString file1, QString file2){

    //open-close
    //S=open SS=close
    //初期化
    MAX_V=-100000;
    MIN_V=100000;
    //ファイル読み込み


    cout<<"file1"<<endl;
    cout<<file1.toStdString()<<endl;
    cout<<"file2"<<endl;
    cout<<file2.toStdString()<<endl;
    if (file1.contains("close", Qt::CaseInsensitive)){
        QString tmpo;
        tmpo=file1;
        file1=file2;
        file2=tmpo;
        //    cout<<"includingclose"<<endl;
    }


    fstream ifs1;//open
    fstream ifs2;//close

    ifs1.open(&file1.toStdString()[0],ios::in | ios::binary);
    ifs2.open(&file2.toStdString()[0],ios::in | ios::binary);

    int Count( 0 );
    int i=0;
    int j=0;
    char Data;
    double tmp;


    while(1){
        while( !ifs1.fail() )//open
        {
            //縦*横*２バイト
            if( Count >= Height*Width*2)  break;  // 行数

            ifs1.read( &Data,1);

            //buf[0] buf[1] → buf[1]*256+buf[0]と2biteをリトルエンディアンで変換
            //1バイト目r
            //データ表示 cout <<(int)(0xff&Data)<<endl;

            tmp=0xff&Data;

            //2バイト目
            ++Count;

            ifs1.read( &Data,1);
            //cout <<(int)(0xff&Data)<<endl;

            tmp+=(0xff&Data)*256;
            // tmp=tmp/8;
            image[i][j]=tmp;
            //  cout<<"f1tmp"<<endl;
            //cout<<tmp<<endl;



            j++;

            if( Count % (Width*2) == Width*2-1 ){
                i++;
                j=0;
            }

            ++Count;
        }

        ifs1.close();

        Count = 0;
        i=0;
        j=0;

        while( !ifs2.fail() )//close
        {
            //縦*横*２バイト
            if( Count >= Height*Width*2)  break;  // 行数

            ifs2.read( &Data,1);

            //buf[0] buf[1] → buf[1]*256+buf[0]と2biteをリトルエンディアンで変換
            //1バイト目
            //データ表示 cout <<(int)(0xff&Data)<<endl;

            tmp=0xff&Data;

            //2バイト目
            ++Count;

            ifs2.read( &Data,1);
            //cout <<(int)(0xff&Data)<<endl;

            tmp+=(0xff&Data)*256;

            //ここからclose画像を引く
            //  tmp=tmp/8;
            image[i][j] -= tmp;

            //  cout<<"image"<<endl;
            //cout<<image[i][j]<<endl;

            if(image[i][j]>MAX_V)MAX_V=image[i][j];
            if(image[i][j]<MIN_V)MIN_V=image[i][j];

            j++;

            if( Count % (Width*2) == Width*2-1 ){
                i++;
                j=0;
            }

            ++Count;
        }

        ifs2.close();


        //FITS画像、ヘッダ出力用
        long naxis    =   2;
        long naxes[2] = { 384, 256 };
        std::auto_ptr<FITS> pFits(0);
        std::auto_ptr<FITS> pFits1(0);
        try
        {

            QString initialFileDirectory = QFileDialog::getExistingDirectory(this,tr("Select save Folder"),"/Applications/HEATcalibration");
            if(initialFileDirectory == ""){
                return;

            }

            QString filename1=QFileInfo(file1).fileName();
            QString filename2=QFileInfo(file2).fileName();

            const std::string fileNametemp(initialFileDirectory.toStdString()+"/"+filename1.toStdString()+"_"+filename2.toStdString()+"_openclose.fit");

            pFits.reset( new FITS(fileNametemp,DOUBLE_IMG , naxis , naxes ) );

        }
        catch (FITS::CantCreate)
        {
            return;
        }

        int nelements(1);
        nelements = std::accumulate(&naxes[0],&naxes[naxis],1,std::multiplies<double>());
        std::valarray<double> arraytemp(nelements);

        int k=0;
        for (int i = 0; i < 256; i++){
            for(int j=0;j<384;j++){

                arraytemp[k]=image[i][j];
                k++;
            }
        }
        int  fpixel(1);

        pFits->pHDU().write(fpixel,nelements,arraytemp);






        /*


        //中央の値がオプティカルブラック領域よりも高いことを確認
        //低い場合は逆にして計算
        double AveOB=0,AveCenter=0;
        int countB=0,countC=0;
        for(int x=-2;x<=2;x++){
            for(int y=-2;y<=2;y++){
                //異常値判定
                if(image[8+y][8+x]<image[8][8]+ 10
                        && image[8+y][8+x]>image[8][8]- 10){
                    AveOB += image[8+y][8+x];
                    //異常値を除いた積算回数
                    countB++;
                }
            }
        }
        AveOB/=(double)countB;

        for(int x=-10;x<=10;x++){
            for(int y=-10;y<=10;y++){
                //異常値判定
                if(image[128+y][163+x]<image[128][163]+10
                        && image[128+y][163+x]>image[128][163]-10){
                    AveCenter += image[128+y][163+x];
                    //異常値を除いた積算回数
                    countC++;
                }
            }
        }
        AveCenter/=(double)countC;

        if(AveCenter < AveOB){
            ifs1.open(&file2.toStdString()[0],ios::in | ios::binary);
            ifs2.open(&file1.toStdString()[0],ios::in | ios::binary);
            i=0;
            j=0;
            Count=0;
        }



        //else break;
*/
        break;
    }


    //カラーバー作成
    makeColorTable();
    //反映
    this->update();

}


void ShowImage::SetDarkImage(QString getdarkfilename)
{
    valarray<double> contents;
    auto_ptr<FITS> pInfile(0);
    QString appPath;
    appPath = QCoreApplication::applicationDirPath();

    cout<<getdarkfilename.toStdString()<<endl;

    QString darkfilename;
    darkfilename=appPath+"/"+getdarkfilename;
    try{
        pInfile.reset(new FITS(darkfilename.toStdString().c_str(), Read, true));
        PHDU& fitsImage=pInfile->pHDU();
        fitsImage.read(contents);
        fitsImage.readAllKeys();

        int counter=0;
        double tmp1=0;

        for(int i=0; i<Height; i++){
            for(int j=0; j<Width; j++){
                tmp1=contents[counter];

                //  tmp1=tmp1/8;
                darkimage[i][j]=tmp1;
                counter++;
                cout<<darkimage[i][j]<<endl;
            }
        }
    }

    catch(FITS::CantCreate)
    {
        cout<<"Can't open fits image file"<<endl;
    }
}

void ShowImage::initializedarkimage(){
    for(int i=0; i<Height; i++){
        for(int j=0; j<Width; j++){
            darkimage[i][j]=-1000;
            cout<<darkimage[i][j]<<endl;
        }
    }
}

int ShowImage::GetAsteroidPixel(int i, int j){
    if(image[i][j]-darkimage[i][j]>-50)
    {
        return 1;
    }
    else{
        return 0;
    }
}








void ShowImage::calibrateImageforBlackbodyAllPixel(QString s, int x, int y){//変換

    renderunitflag=false;

    double h = s.section(',',-2,-2).toDouble();
    double g = s.section(',',-3,-3).toDouble();
    double DN = image[y+6][x+16];
    //  double DN = image[y+2][x+16];以前
    double DN2=0;
    double tmp1=0;
    //double FT=0;
    double FT1=0;


    //DN-ダークがDN>50を満たす場合は計算する
    //それ以外は計算しない
    // DN2=DN-darkimage[y+6][x+16];
    //cout<<DN2<<endl;

    /*
     *
     * if(DN2>50){

        FT1=round1((DN-h)/g);
        tmp1 = gettemperature(FT1);
        //HEAT上で出力
        calibrationImage[y+6][x+16] = tmp1 + 273.15;
        //FITSファイル出力
        fitstemperature2[y][x] = tmp1 + 273.15;
    }
    else{
        //HEAT上で出力
        calibrationImage[y+6][x+16] =68.15;
        //FITSファイル出力
        fitstemperature2[y][x] = 68.15;
    }
*/


    FT1=round1((DN-h)/g);
    tmp1 = gettemperature(FT1);
    //HEAT上で出力
    calibrationImage[y+6][x+16] = tmp1 + 273.15;
    //FITSファイル出力
    //fitstemperature2[y][x] = tmp1 + 273.15;
    fitstemperature2[y][x] = h;


    /*
    int count=-400;
    double yy=0;
    double xx=-400;

    for(xx=-400;xx<600;xx++){
        cout<<"else if(FT1<=";  cout<< round1(planck((xx+0.9+273.15))); cout<<"){"; cout<<endl;
        cout<<"if(FT1==";  cout<< round1(planck((count+273.15))); cout<<")tmp1="; cout<<xx;   cout<<";"; cout<<endl;
        for(yy=count+0.1;yy<count+1.0;yy=yy+0.1){
            if(yy==xx+1)break;
            else
            {
                cout<<"else if(FT1<=";  cout<< round1(planck((yy+273.15))); cout<<")tmp1="; cout<<yy;   cout<<";";
                cout<<endl;
            }
        }
        count=count+1;
        cout<<"}";
        cout<<endl;
    }
*?



    /*
    QFile file("/Users/sukokentarou/Desktop/HEATdata.txt");
    file.open(QIODevice::Append);
    QTextStream out(&file);
    out<<"x: ";    out<<x<<endl;
    out<<"y: ";    out<<y<<endl;
    out<<"DN: ";    out<<DN<<endl;
    out<<"ケルビン: ";    out<<calibrationImage[y+6][x+16]<<endl;
    file.close();
*/

}



void ShowImage::confirmation(QString s, int x, int y, QString subFileName1){//温度差計算
    fstream ifs;
    ifs.open(&subFileName1.toStdString()[0],ios::in | ios::binary);

    QFileInfo fileinfo;
    fileinfo.setFile(subFileName1);
    QString ext = fileinfo.suffix();
    ext=ext.toLower();


    if (ext == "fit" || ext=="fits" || ext=="fts")//(fitファイルの検索)
    {
        valarray<long> contents;
        auto_ptr<FITS> pInfile(0);
        try{
            pInfile.reset(new FITS(subFileName1.toStdString().c_str(), Read, true));
            PHDU& fitsImage=pInfile->pHDU();
            int fitsWidth=fitsImage.axis(0);//横軸よみとり
            int fitsHeight=fitsImage.axis(1);//縦軸よみとり
            fitsImage.read(contents);

            int counter=0;
            double tmp1=0;

            for(int i=0; i<fitsHeight; i++)
            {
                for(int j=0; j<fitsWidth; j++)
                {

                    tmp1=contents[counter];
                    fitsimagesub[fitsHeight-i-1][j]=tmp1;

                    counter++;
                }
            }
        }

        catch(FITS::CantCreate)
        {
            cout<<"Can't open fits image file"<<endl;
            return ;
        }
    }



    //ここを変えて誤差をみる
    calibrationImage[y+2][x+16]=fitsimagesub[y][x];
}


void ShowImage::calibrateImagetoRadianceforBlackbodyAllPixel(QString s, int x, int y){//輝度放射計算

    double h = s.section(',',-2,-2).toDouble();
    double g = s.section(',',-3,-3).toDouble();
    double DN = image[y+2][x+16];
    double DN2=0;
    double tmp1=0;
    double FT1;
    double epsilon=0.925;
    double Radiance=0;

    //DN-ダークがDN>50を満たす場合は計算する
    //それ以外は計算しない
    DN2=DN-darkimage[y+6][x+16];
    // cout<<darkimage[y+6][x+16]<<endl;

    if(DN2>50){

        FT1=round1((DN-h)/g);
        tmp1 = gettemperature(FT1);
        Radiance = epsilon *((tmp1+273.15)*(tmp1+273.15)*(tmp1+273.15)*(tmp1+273.15))/PI;//輝度放射計算
    }
    else{
        Radiance=0;
    }



    //HEAT上で出力
    calibrationImage[y+2][x+16] = Radiance;

    //FITSファイル出力
    fitstemperature2[y][x] = Radiance;

}


double ShowImage::getPixelValue(int y, int x){

    return image[y][x];

}

//横軸ピクセル数、縦軸DN値のグラフ表示関数
void ShowImage::drawPixcelLineGraph(QString heightValue){

    QVector<double> w(Width), DN(Width); //グラフ描画用
    bool checkInt = false; //Qstring->Int変換判定用
    int y = heightValue.toInt(&checkInt,10);//Qstring->Int変換かつ不正な値ではないか判定

    if(0 <= y && y <= Width && checkInt == true){
        for(int i = 0; i < Width; i++)
            w[i] = i;

        for(int i = 0; i < Width; i++)
            DN[i] = this->getPixelValue(y,i);

    }else{
        //printf("値が不正です");
    }

    pg.drawGraph(w,DN);
    pg.show();
}


//四捨五入計算関数
double ShowImage:: round3(double dIn)
{
    double    dOut;

    dOut = dIn * pow(10.0, 3);
    if(dIn>=0){
        dOut = (double)(int)(dOut + 0.5);
    }
    else dOut = (double)(int)(dOut - 0.5);
    return dOut * pow(10.0, -3);
}

double ShowImage:: round2(double dIn)
{
    double    dOut;

    dOut = dIn * pow(10.0, 2);
    if(dIn>=0){
        dOut = (double)(int)(dOut + 0.5);
    }
    else dOut = (double)(int)(dOut - 0.5);
    return dOut * pow(10.0, -2);
}

double ShowImage:: round1(double dIn)
{
    double    dOut;

    dOut = dIn * pow(10.0, 5);
    if(dIn>=0){
        dOut = (double)(int)(dOut + 0.5);
    }
    else dOut = (double)(int)(dOut - 0.5);
    return dOut * pow(10.0, -5);
}


//7次回帰式を使う温度変換
void ShowImage::calibrateImage(QString s, int x, int y){

    double h = s.section(',',-2,-2).toDouble();
    double g = s.section(',',-3,-3).toDouble();
    double f = s.section(',',-4,-4).toDouble();
    double e = s.section(',',-5,-5).toDouble();
    double d = s.section(',',-6,-6).toDouble();
    double c = s.section(',',-7,-7).toDouble();
    double b = s.section(',',-8,-8).toDouble();
    double a = s.section(',',-9,-9).toDouble();

    double DN = image[y+2][x+16];
    double tmp=0;

    tmp = a*pow(DN,7) + b*pow(DN,6) + c*pow(DN,5) + d*pow(DN,4) + e*pow(DN,3) + f*pow(DN,2) + g*DN + h;
    calibrationImage[y+2][x+16] = tmp;
    //qDebug() << tmp;
}
void ShowImage::initializeFITSarray(){
    for(int i=0; i<248; i++){
        for(int j=0; j<328; j++){
            fitstemperature2[i][j]=68.15;
        }
    }
}


void ShowImage::updateImage(int judge,QString dirpath,QString fitdirectory){

    MAX_V = -100000;
    MIN_V = 100000;
    //judgeが0のとき温度画像出力
    if(judge==0){


        for(int i=0; i<Height; i++){
            for(int j=0; j<Width; j++){

                if(5<i && i<253 && 15 < j && j<343){
                    if(MAX_V < calibrationImage[i][j]){
                        MAX_V = calibrationImage[i][j];
                    }

                    if(MIN_V > calibrationImage[i][j]){
                        MIN_V = calibrationImage[i][j];
                    }

                    image[i][j] = calibrationImage[i][j];
                }
            }
        }

        for(int i=0; i<Height; i++){
            for(int j=0; j<Width; j++){
                if(!(5<i && i<253 && 15 < j && j<343)){
                    image[i][j] = MIN_V - ((MAX_V - MIN_V)/10);
                }
            }
        }
        int k=0;
        for(int i=0; i<248; i++){
            for(int j=0; j<328; j++){

                fitstemperature[k] =fitstemperature2[i][j];
                //                fitstemperature[k] =fitstemperature2[248-1-i][j];
                k++;
            }
        }



        //カラーバー作成
        makeColorTable();
        //反映
        this->update();
    }



    //judgeが1とのき輝度出力
    else if(judge==1)
    {
        double epsilon=0.925;
        double sigma=5.67032*pow(10,-8);

        for(int i=0; i<Height; i++){
            for(int j=0; j<Width; j++){

                if(5<i && i<253 && 15 < j && j<343){
                    if(MAX_V < (epsilon*sigma*pow(calibrationImage[i][j],4))/PI){
                        MAX_V = (epsilon*sigma*pow(calibrationImage[i][j],4))/PI;
                    }

                    if(MIN_V > (epsilon*sigma*pow(calibrationImage[i][j],4))/PI){
                        MIN_V = (epsilon*sigma*pow(calibrationImage[i][j],4))/PI;
                    }

                    image[i][j] = (epsilon*sigma*pow(calibrationImage[i][j],4))/PI;
                }
            }
        }

        for(int i=0; i<Height; i++){
            for(int j=0; j<Width; j++){
                if(!(5<i && i<253 && 15 < j && j<343)){
                    image[i][j] = MIN_V - ((MAX_V - MIN_V)/10);
                }
            }
        }
        int k=0;
        for(int i=0; i<248; i++){
            for(int j=0; j<328; j++){
                fitstemperature[k] =fitstemperature2[i][j];
                //fitstemperature[k] =fitstemperature2[248-1-i][j];
                k++;
            }
        }

        //カラーバー作成
        makeColorTable();
        //反映
        this->update();
    }



    //FITS画像、ヘッダ出力用
    long naxis    =   2;
    // long naxes[2] = { 384, 256 };
    long naxes[2] = { 328, 248 };
    std::auto_ptr<FITS> pFits(0);
    std::auto_ptr<FITS> pFits1(0);
    try
    {

        //QString fitdirectory= QFileDialog::getExistingDirectory(this, tr("Select the directory to save the image"),"/Applications/HEAT_DATA");
        if(fitdirectory == ""){
            return;
        }

        //fitsファイル名用に日付のつけなおし

        QString dateconversion=QString::fromStdString(date);
        QString YYYY,MM,DD,hh,mm,ss;
        QString YMD,hms;



        YYYY=dateconversion.mid(0,4);
        MM=dateconversion.mid(5,2);
        DD=dateconversion.mid(8,2);
        hh=dateconversion.mid(11,2);
        mm=dateconversion.mid(14,2);
        ss=dateconversion.mid(17,2);
/*
        cout<<YYYY.toStdString()<<endl;
        cout<<MM.toStdString()<<endl;
        cout<<DD.toStdString()<<endl;
        cout<<hh.toStdString()<<endl;
        cout<<mm.toStdString()<<endl;
        cout<<ss.toStdString()<<endl;
*/
        YMD=YYYY+MM+DD;
        hms=hh+mm+ss;


        QString fitfilename=QFileInfo(filename).fileName();
        QString fitfilenamewithoutsuffix = QFileInfo(fitfilename).baseName();


        if(fitfilenamewithoutsuffix.contains("_l1",Qt::CaseInsensitive)==1){
            fitfilenamewithoutsuffix.replace("_l1",qgetenv(""));
        }
        //const std::string fileName("!"+fitdirectory.toStdString()+"/"+fitfilename.toStdString()+"_temperature.fit");
        const std::string fileNametemp("!"+fitdirectory.toStdString()+"/"+fitfilenamewithoutsuffix.toStdString()+"_l2a.fit");
        const std::string fileNameRadiance("!"+fitdirectory.toStdString()+"/"+fitfilenamewithoutsuffix.toStdString()+"_l2b.fit");
        //const std::string fileNametemp("!"+fitdirectory.toStdString()+"/hyb2_tir__"+fitfilenamewithoutsuffix.toStdString()+"_l2a.fit");
        //const std::string fileNameRadiance("!"+fitdirectory.toStdString()+"/hyb2_tir__"+fitfilenamewithoutsuffix.toStdString()+"_l2b.fit");
        //  pFits.reset( new FITS(fileNametemp, SHORT_IMG , naxis , naxes ) );
        //pFits1.reset( new FITS(fileNameRadiance, SHORT_IMG , naxis , naxes ) );
        pFits.reset( new FITS(fileNametemp,DOUBLE_IMG , naxis , naxes ) );
        pFits1.reset( new FITS(fileNameRadiance, DOUBLE_IMG , naxis , naxes ) );


    }
    catch (FITS::CantCreate)
    {
        return;
    }

    int nelements(1);
    nelements = std::accumulate(&naxes[0],&naxes[naxis],1,std::multiplies<double>());
    std::valarray<double> arraytemp(nelements);

    //for (int i = 0; i < 384*256; i++)
    for (int i = 0; i < 328*248; i++)
    {
        arraytemp[i]=fitstemperature[i];
    }
    int  fpixel(1);
    pFits->pHDU().addKey("ORIGIN",origin,"organization responsible for the data");
    pFits->pHDU().addKey("DATE",date,"date of generation of HDU in UTC");
    pFits->pHDU().addKey("DATE-BEG",date_beg,"start date of observation program (UTC)");
    pFits->pHDU().addKey("DATE-OBS",date_obs,"start date of observation (UTC)");
    pFits->pHDU().addKey("DATE-END",date_end,"end date of observation (UTC)");
    pFits->pHDU().addKey("TELESCOP",telescop,"telescope used to acquire data");
    pFits->pHDU().addKey("INSTRUME",instrume,"name of instrument");
    pFits->pHDU().addKey("OBJECT",object,"name of observed object");
    pFits->pHDU().addKey("BUNIT",bunit,"physical unit of array values");
    pFits->pHDU().writeComment("");
    pFits->pHDU().writeComment("***** TIR Common Information *****");
    pFits->pHDU().writeComment("");
    pFits->pHDU().addKey("XPOSURE",QString::number(xposure).toStdString(),"exposure time [sec]");
    pFits->pHDU().addKey("IFOV",QString::number(ifov).toStdString(),"instantaneous field of view [rad]");
    pFits->pHDU().addKey("FILTER",filter,"bandpath range of filter (um)");
    pFits->pHDU().writeComment("");
    pFits->pHDU().writeComment("***** Observation Program *****");
    pFits->pHDU().writeComment("");
    pFits->pHDU().addKey("OPRGNAME",oprgname,"observation program name");
    pFits->pHDU().addKey("OPRGNO",oprgno,"observation program number");
    pFits->pHDU().writeComment("");
    pFits->pHDU().writeComment("***** TIR Image Information *****");
    pFits->pHDU().writeComment("");
    pFits->pHDU().addKey("ROI_LLX",QString::number(roi_llx).toStdString(),"x lower-left corner pixel of image ");
    pFits->pHDU().addKey("ROI_LLY",QString::number(roi_lly).toStdString(),"y lower-left corner pixel of image");
    pFits->pHDU().addKey("ROI_URX",QString::number(roi_urx).toStdString(),"x upper-right corner pixel of image");
    pFits->pHDU().addKey("ROI_URY",QString::number(roi_ury).toStdString(),"y upper-right corner pixel of image");
    pFits->pHDU().addKey("DATAMAX",QString::number(datamax).toStdString(),"maximum data value");
    pFits->pHDU().addKey("DATAMIN",QString::number(datamin).toStdString(),"minimum data value");
    pFits->pHDU().addKey("MEAN",QString::number(mean).toStdString(),"mean value of the data");
    pFits->pHDU().addKey("STDEV",QString::number(stdev).toStdString(),"standard deviation of the data");
    pFits->pHDU().addKey("MISS_VAL",QString::number(miss_val).toStdString(),"flag value of missing pixel");
    pFits->pHDU().addKey("MISS_NUM",QString::number(miss_num).toStdString(),"number of missing pixel");
    pFits->pHDU().addKey("DEAD_VAL",QString::number(dead_val).toStdString(),"flag value of dead pixel");
    pFits->pHDU().addKey("DEAD_NUM",QString::number(dead_num).toStdString(),"number of dead pixel");
    pFits->pHDU().addKey("SATU_VAL",QString::number(satu_val).toStdString(),"flag value of saturated pixel");
    pFits->pHDU().addKey("SATU_NUM",QString::number(satu_num).toStdString(),"number of saturated pixels");
    pFits->pHDU().addKey("IMGCMPRV",imgcmprv,"compression rev.: RAW_DAT/LOSSLESS/LOSSY");
    pFits->pHDU().addKey("IMGCMPAL",imgcmpal,"compression alg.: JPEG2000/STAR_PIXEL");
    pFits->pHDU().addKey("IMGCMPPR",imgcmppr,"compression parameter");
    pFits->pHDU().addKey("IMG_ERR",QString::number(img_err).toStdString(),"onboard image proc. return status");
    pFits->pHDU().writeComment("");
    pFits->pHDU().writeComment("***** TIR Telemetry *****");
    pFits->pHDU().writeComment("");
    pFits->pHDU().addKey("IMGSEQC",imgseqc,"image sequence counter");
    pFits->pHDU().addKey("IMGACCM",QString::number(imgaccm).toStdString(),"number of accumulated images");
    pFits->pHDU().addKey("BITDEPTH",QString::number(bitdepth).toStdString(),"image bit depth");
    pFits->pHDU().addKey("PLT_POW",plt_pow,"peltier ON/OFF");
    pFits->pHDU().addKey("PLT_STAT",plt_stat,"peltier status");
    pFits->pHDU().addKey("BOL_STAT",bol_stat,"bolometer status");
    pFits->pHDU().addKey("BOL_TRGT",QString::number(bol_trgt).toStdString(),"bolometer calibration target");
    pFits->pHDU().addKey("BOL_RANG",QString::number(bol_rang).toStdString(),"bolometer calibration range");
    pFits->pHDU().addKey("BOL_TEMP",QString::number(bol_temp).toStdString(),"bolometer temperature [degC]");
    pFits->pHDU().addKey("PKG_TEMP",QString::number(pkg_temp).toStdString(),"package temperature [degC]");
    pFits->pHDU().addKey("CAS_TEMP",QString::number(cas_temp).toStdString(),"case temperature [degC]");
    pFits->pHDU().addKey("SHT_TEMP",QString::number(sht_temp).toStdString(),"shutter temperature [degC]");
    pFits->pHDU().addKey("LEN_TEMP",QString::number(len_temp).toStdString(),"lens temperature [degC]");
    pFits->pHDU().addKey("BGR_VOL",QString::number(bgr_vol).toStdString(),"BGR voltage [V]");
    pFits->pHDU().addKey("VB1_VOL",QString::number(vb1_vol).toStdString(),"VB1 voltage [V]");
    pFits->pHDU().addKey("ADOFSVOL",QString::number(adofsvol).toStdString(),"A/D_OFS voltage [V]");
    pFits->pHDU().addKey("HCE_TEMP",QString::number(hce_temp).toStdString(),"HCE TIR sensor temperature [degC]");
    pFits->pHDU().addKey("PNL_TEMP",QString::number(pnl_temp).toStdString(),"HCE TIR sensor panel temperature [degC]");
    pFits->pHDU().addKey("AE_TEMP",QString::number(ae_temp).toStdString(),"HCE TIR analog electronics temperature [degC]");
    pFits->pHDU().writeComment("");
    pFits->pHDU().writeComment("***** Observation Information by SPICE kernel *****");
    pFits->pHDU().writeComment("");
    pFits->pHDU().addKey("S_DISTHT",QString::number(s_distht).toStdString(),"distance between HYB2 and the target [km]");
    pFits->pHDU().addKey("S_DISTHE",QString::number(s_disthe).toStdString(),"distance between HYB2 and Earth [km]");
    pFits->pHDU().addKey("S_DISTHS",QString::number(s_disths).toStdString(),"distance between HYB2 and Sun [km]");
    pFits->pHDU().addKey("S_DISTTS",QString::number(s_distts).toStdString(),"distance between the target and Sun [km]");
    pFits->pHDU().addKey("S_TGRADI",QString::number(s_tgradi).toStdString(),"the target radius at the equator [km]");
    pFits->pHDU().addKey("S_APPDIA",QString::number(s_appdia).toStdString(),"apparent diameter of the target [deg]");
    pFits->pHDU().addKey("S_SOLLAT",QString::number(s_sollat).toStdString(),"sub solar latitude [deg] of the target");
    pFits->pHDU().addKey("S_SOLLON",QString::number(s_sollon).toStdString(),"sub solar longitude [deg] of the target");
    pFits->pHDU().addKey("S_SSCLAT",QString::number(s_ssclat).toStdString(),"sub S/C latitude [deg] of the target");
    pFits->pHDU().addKey("S_SSCLON",QString::number(s_ssclon).toStdString(),"sub S/C longitude [deg] of the target");
    pFits->pHDU().addKey("S_SSCLST",QString::number(s_ssclst).toStdString(),"sub S/C local solar time [h] of the target");
    pFits->pHDU().addKey("S_SSCPX",QString::number(s_sscpx).toStdString(),"sub S/C position on Image Array (axis1)");
    pFits->pHDU().addKey("S_SSCPY",QString::number(s_sscpy).toStdString(),"sub S/C position on Image Array (axis2)");
    pFits->pHDU().addKey("S_SCXSAN",QString::number(s_scxsan).toStdString(),"angle of S/C X axis and Sun direction [deg]");
    pFits->pHDU().addKey("S_SCYSAN",QString::number(s_scysan).toStdString(),"angle of S/C Y axis and Sun direction [deg]");
    pFits->pHDU().addKey("S_SCZSAN",QString::number(s_sczsan).toStdString(),"angle of S/C Z axis and Sun directino [deg]");
    pFits->pHDU().writeComment("");
    pFits->pHDU().writeComment("***** SPICE Kernels *****");
    pFits->pHDU().writeComment("");
    pFits->pHDU().addKey("NAIFNAME",naifname,"SPICE instrument name");
    pFits->pHDU().addKey("NAIFID",QString::number(naifid).toStdString(),"SPICE instrument ID");
    pFits->pHDU().addKey("MKNAME",mkname,"SPICE Meta kernel name");
    pFits->pHDU().writeComment("");
    pFits->pHDU().writeComment("***** Version ***** ");
    pFits->pHDU().writeComment("");
    pFits->pHDU().addKey("VERSION",QString::number(version).toStdString(),"version of the HDU");

    /*
    //FITSヘッダに使った地上実験ファイル名を列挙する場合に使用
    QFile filex(dirpath+"/UsedImage.txt");
    if(filex.exists()){
        filex.open(QIODevice::ReadOnly);
        QTextStream load(&filex);

        int i=0;
        QString usedimage[5000];

        while(!load.atEnd()){
            usedimage[i]= load.readLine();
            pFits->pHDU().addKey("IMAGE"+QString::number(i).toStdString(),usedimage[i].toStdString(),"used ground data");
            i++;
        }
        filex.close();
    }

  */


    pFits->pHDU().write(fpixel,nelements,arraytemp);


    //輝度画像出力
    int nelements1(1);
    nelements1 = std::accumulate(&naxes[0],&naxes[naxis],1,std::multiplies<double>());
    std::valarray<double> arrayRadiance(nelements1);

    double epsilon=0.925;
    double sigma=5.67032*pow(10,-8);
    for (int i = 0; i < 328*248; i++)
        //    for (int i = 0; i < 384*256; i++)

    {
        arrayRadiance[i]=(epsilon*sigma*pow(fitstemperature[i],4))/PI;
    }
    int  fpixel1(1);
    pFits1->pHDU().addKey("ORIGIN",origin,"organization responsible for the data");
    pFits1->pHDU().addKey("DATE",date,"date of generation of HDU in UTC");
    pFits1->pHDU().addKey("DATE-BEG",date_beg,"start date of observation program (UTC)");
    pFits1->pHDU().addKey("DATE-OBS",date_obs,"start date of observation (UTC)");
    pFits1->pHDU().addKey("DATE-END",date_end,"end date of observation (UTC)");
    pFits1->pHDU().addKey("TELESCOP",telescop,"telescope used to acquire data");
    pFits1->pHDU().addKey("INSTRUME",instrume,"name of instrument");
    pFits1->pHDU().addKey("OBJECT",object,"name of observed object");
    pFits1->pHDU().addKey("BUNIT",bunit,"physical unit of array values");
    pFits1->pHDU().writeComment("");
    pFits1->pHDU().writeComment("***** TIR Common Information *****");
    pFits1->pHDU().writeComment("");
    pFits1->pHDU().addKey("XPOSURE",QString::number(xposure).toStdString(),"exposure time [sec]");
    pFits1->pHDU().addKey("IFOV",QString::number(ifov).toStdString(),"instantaneous field of view [rad]");
    pFits1->pHDU().addKey("FILTER",filter,"bandpath range of filter (um)");
    pFits1->pHDU().writeComment("");
    pFits1->pHDU().writeComment("***** Observation Program *****");
    pFits1->pHDU().writeComment("");
    pFits1->pHDU().addKey("OPRGNAME",oprgname,"observation program name");
    pFits1->pHDU().addKey("OPRGNO",oprgno,"observation program number");
    pFits1->pHDU().writeComment("");
    pFits1->pHDU().writeComment("***** TIR Image Information *****");
    pFits1->pHDU().writeComment("");
    pFits1->pHDU().addKey("ROI_LLX",QString::number(roi_llx).toStdString(),"x lower-left corner pixel of image ");
    pFits1->pHDU().addKey("ROI_LLY",QString::number(roi_lly).toStdString(),"y lower-left corner pixel of image");
    pFits1->pHDU().addKey("ROI_URX",QString::number(roi_urx).toStdString(),"x upper-right corner pixel of image");
    pFits1->pHDU().addKey("ROI_URY",QString::number(roi_ury).toStdString(),"y upper-right corner pixel of image");
    pFits1->pHDU().addKey("DATAMAX",QString::number(datamax).toStdString(),"maximum data value");
    pFits1->pHDU().addKey("DATAMIN",QString::number(datamin).toStdString(),"minimum data value");
    pFits1->pHDU().addKey("MEAN",QString::number(mean).toStdString(),"mean value of the data");
    pFits1->pHDU().addKey("STDEV",QString::number(stdev).toStdString(),"standard deviation of the data");
    pFits1->pHDU().addKey("MISS_VAL",QString::number(miss_val).toStdString(),"flag value of missing pixel");
    pFits1->pHDU().addKey("MISS_NUM",QString::number(miss_num).toStdString(),"number of missing pixel");
    pFits1->pHDU().addKey("DEAD_VAL",QString::number(dead_val).toStdString(),"flag value of dead pixel");
    pFits1->pHDU().addKey("DEAD_NUM",QString::number(dead_num).toStdString(),"number of dead pixel");
    pFits1->pHDU().addKey("SATU_VAL",QString::number(satu_val).toStdString(),"flag value of saturated pixel");
    pFits1->pHDU().addKey("SATU_NUM",QString::number(satu_num).toStdString(),"number of saturated pixels");
    pFits1->pHDU().addKey("IMGCMPRV",imgcmprv,"compression rev.: RAW_DAT/LOSSLESS/LOSSY");
    pFits1->pHDU().addKey("IMGCMPAL",imgcmpal,"compression alg.: JPEG2000/STAR_PIXEL");
    pFits1->pHDU().addKey("IMGCMPPR",imgcmppr,"compression parameter");
    pFits1->pHDU().addKey("IMG_ERR",QString::number(img_err).toStdString(),"onboard image proc. return status");
    pFits1->pHDU().writeComment("");
    pFits1->pHDU().writeComment("***** TIR Telemetry *****");
    pFits1->pHDU().writeComment("");
    pFits1->pHDU().addKey("IMGSEQC",imgseqc,"image sequence counter");
    pFits1->pHDU().addKey("IMGACCM",QString::number(imgaccm).toStdString(),"number of accumulated images");
    pFits1->pHDU().addKey("BITDEPTH",QString::number(bitdepth).toStdString(),"image bit depth");
    pFits1->pHDU().addKey("PLT_POW",plt_pow,"peltier ON/OFF");
    pFits1->pHDU().addKey("PLT_STAT",plt_stat,"peltier status");
    pFits1->pHDU().addKey("BOL_STAT",bol_stat,"bolometer status");
    pFits1->pHDU().addKey("BOL_TRGT",QString::number(bol_trgt).toStdString(),"bolometer calibration target");
    pFits1->pHDU().addKey("BOL_RANG",QString::number(bol_rang).toStdString(),"bolometer calibration range");
    pFits1->pHDU().addKey("BOL_TEMP",QString::number(bol_temp).toStdString(),"bolometer temperature [degC]");
    pFits1->pHDU().addKey("PKG_TEMP",QString::number(pkg_temp).toStdString(),"package temperature [degC]");
    pFits1->pHDU().addKey("CAS_TEMP",QString::number(cas_temp).toStdString(),"case temperature [degC]");
    pFits1->pHDU().addKey("SHT_TEMP",QString::number(sht_temp).toStdString(),"shutter temperature [degC]");
    pFits1->pHDU().addKey("LEN_TEMP",QString::number(len_temp).toStdString(),"lens temperature [degC]");
    pFits1->pHDU().addKey("BGR_VOL",QString::number(bgr_vol).toStdString(),"BGR voltage [V]");
    pFits1->pHDU().addKey("VB1_VOL",QString::number(vb1_vol).toStdString(),"VB1 voltage [V]");
    pFits1->pHDU().addKey("ADOFSVOL",QString::number(adofsvol).toStdString(),"A/D_OFS voltage [V]");
    pFits1->pHDU().addKey("HCE_TEMP",QString::number(hce_temp).toStdString(),"HCE TIR sensor temperature [degC]");
    pFits1->pHDU().addKey("PNL_TEMP",QString::number(pnl_temp).toStdString(),"HCE TIR sensor panel temperature [degC]");
    pFits1->pHDU().addKey("AE_TEMP",QString::number(ae_temp).toStdString(),"HCE TIR analog electronics temperature [degC]");
    pFits1->pHDU().writeComment("");
    pFits1->pHDU().writeComment("***** Observation Information by SPICE kernel *****");
    pFits1->pHDU().writeComment("");
    pFits1->pHDU().addKey("S_DISTHT",QString::number(s_distht).toStdString(),"distance between HYB2 and the target [km]");
    pFits1->pHDU().addKey("S_DISTHE",QString::number(s_disthe).toStdString(),"distance between HYB2 and Earth [km]");
    pFits1->pHDU().addKey("S_DISTHS",QString::number(s_disths).toStdString(),"distance between HYB2 and Sun [km]");
    pFits1->pHDU().addKey("S_DISTTS",QString::number(s_distts).toStdString(),"distance between the target and Sun [km]");
    pFits1->pHDU().addKey("S_TGRADI",QString::number(s_tgradi).toStdString(),"the target radius at the equator [km]");
    pFits1->pHDU().addKey("S_APPDIA",QString::number(s_appdia).toStdString(),"apparent diameter of the target [deg]");
    pFits1->pHDU().addKey("S_SOLLAT",QString::number(s_sollat).toStdString(),"sub solar latitude [deg] of the target");
    pFits1->pHDU().addKey("S_SOLLON",QString::number(s_sollon).toStdString(),"sub solar longitude [deg] of the target");
    pFits1->pHDU().addKey("S_SSCLAT",QString::number(s_ssclat).toStdString(),"sub S/C latitude [deg] of the target");
    pFits1->pHDU().addKey("S_SSCLON",QString::number(s_ssclon).toStdString(),"sub S/C longitude [deg] of the target");
    pFits1->pHDU().addKey("S_SSCLST",QString::number(s_ssclst).toStdString(),"sub S/C local solar time [h] of the target");
    pFits1->pHDU().addKey("S_SSCPX",QString::number(s_sscpx).toStdString(),"sub S/C position on Image Array (axis1)");
    pFits1->pHDU().addKey("S_SSCPY",QString::number(s_sscpy).toStdString(),"sub S/C position on Image Array (axis2)");
    pFits1->pHDU().addKey("S_SCXSAN",QString::number(s_scxsan).toStdString(),"angle of S/C X axis and Sun direction [deg]");
    pFits1->pHDU().addKey("S_SCYSAN",QString::number(s_scysan).toStdString(),"angle of S/C Y axis and Sun direction [deg]");
    pFits1->pHDU().addKey("S_SCZSAN",QString::number(s_sczsan).toStdString(),"angle of S/C Z axis and Sun directino [deg]");
    pFits1->pHDU().writeComment("");
    pFits1->pHDU().writeComment("***** SPICE Kernels *****");
    pFits1->pHDU().writeComment("");
    pFits1->pHDU().addKey("NAIFNAME",naifname,"SPICE instrument name");
    pFits1->pHDU().addKey("NAIFID",QString::number(naifid).toStdString(),"SPICE instrument ID");
    pFits1->pHDU().addKey("MKNAME",mkname,"SPICE Meta kernel name");
    pFits1->pHDU().writeComment("");
    pFits1->pHDU().writeComment("***** Version ***** ");
    pFits1->pHDU().writeComment("");
    pFits1->pHDU().addKey("VERSION",QString::number(version).toStdString(),"version of the HDU");


    /*
    //FITSヘッダに使った地上実験ファイル名を列挙する場合に使用
    if(filex.exists()){
        filex.open(QIODevice::ReadOnly);
        QTextStream load(&filex);

        int i=0;
        QString usedimage[5000];

        while(!load.atEnd()){
            usedimage[i]= load.readLine();
            pFits1->pHDU().addKey("IMAGE"+QString::number(i).toStdString(),usedimage[i].toStdString(),"used ground data");
            i++;
        }
        filex.close();
    }
*/

    pFits1->pHDU().write(fpixel1,nelements1,arrayRadiance);
    //    std::cout << pFits1->pHDU() << std::endl;



}



//背景の値変えたい時
void ShowImage::initializeCalibrateImage(){
    for(int i=0; i<Height; i++){
        for(int j=0; j<Width; j++){
            //背景の値変えたい時はここを変える
            calibrationImage[i][j] =68.15;// (MAX_V+MIN_V)/2;
        }
    }
}



//キャリブレーション用
void ShowImage::loadImageD(QVector<double> img){
    int count=0;
    MAX_V=-10000;
    MIN_V=10000;
    for(int i=0;i<Height;i++){
        for(int j =0; j<Width;j++){
            image[i][j]=img[count];
            if(image[i][j]>MAX_V)MAX_V=image[i][j];
            if(image[i][j]<MIN_V)MIN_V=image[i][j];
            count++;
        }
    }

    for(int i=1;;i++){
        if(100*(i-1)<MAX_V&&MAX_V<100*i){
            MAX_V=100*i; break;
        }
    }


    makeColorTable();
    //反映
    this->update();

}

void ShowImage::loadTxtImageD(QString name)
{
    //初期化
    MAX_V=0;
    MIN_V=100000;
    //cout<<name.toStdString()<<endl;確認用

    openTxtImageD(name);

  //  qDebug()<<"1";

    //カラーバー作成
    makeColorTable();
    //反映
    this->update();

}


int ShowImage::getMaxDN(){
    return MAX_V;
}

int ShowImage::getMinDN(){
    return MIN_V;
}


QVector<double> ShowImage::getImageD(){

    QVector<double> tmp;

    for(int i=0;i<256;i++){
        for(int j=0;j<384;j++){
            tmp.append(image[i][j]);
            //qDebug()<<image[i][j];
        }
    }

    return tmp;
}

void ShowImage::openTxtImageD(QString name)
{

    QFile read(name);
    QString tmpS;

    if(!read.open(QIODevice::ReadOnly))
        return;

    int i=0;
    int j=0;
    double tmp;

  //  qDebug()<<"2";

    QTextStream in(&read);


    while( !in.atEnd() )
    {
        in >> tmpS;
        tmp = tmpS.toDouble();


        if(251>i && i>8 && 342>j && j>6){
            if(tmp>MAX_V)MAX_V=tmp;
            if(tmp<MIN_V)MIN_V=tmp;
        }
        image[i][j]=tmp;

        //テスト用
        /*
                    if(tmp ==150)
                        image[i][j]=2.75;
        */
        j++;

        //qDebug()<<j<< tmp;

        if( j / (Width) == 1 ){
            i++;
            j=0;
        }


    }
    //テスト用
    //MIN_V=150;
    //MAX_V=300;
    read.close();

}


double ShowImage::planck(double T){
    //double x0=8e-6, x1=12e-6; //8μm~12μm 領域
    //double x0=1e-8, x1=20e-6; //
    double lambda,Bt,integral=0,epsilon=0.925;
    // 1000=10μm　100000=100μm
    for (int i=1;i<2001;i++) {
        lambda = (double)i*1e-8; // 0.00000001m=0.01μm step
        //フィルタ範囲のみ積分、係数は読み込んだものを使う
        Bt = ((2*h_planck*c_speed*c_speed)/(pow(lambda,5))/(pow(M_E,c2/(lambda*T))-1.0)*tirfilterforshowimage[i-1][1]*epsilon);
        integral+=(Bt);//資料のF(T)の予想されるフラックスのこと
    }
    integral *= 1e-8;//0.00000001
    return integral;
}

//バンドパスフィルタ読み込み
void ShowImage::loadFilter(){

    QString str;
    QString appPath;
    //初期設定ファイル格納用ディレクトリ
    appPath = QCoreApplication::applicationDirPath();

    QFile file(appPath+"/tir_response.txt");//TIRの応答関数R(λ)のデータ


    if (!file.open(QIODevice::ReadOnly))//読込のみでオープンできたかチェック
    {
        printf("tir_response.txt open error\n");
        return;
    }

    QTextStream in(&file);

    //係数読み込み
    for(int i=0;!in.atEnd();i++){
        for(int j=0;j<3;j++){
            in >> str;
            //要素x,y,z
            tirfilterforshowimage[i][j] = str.toDouble();
        }
    }
}

void ShowImage::connectDB(){
    db.open();

    query = QSqlQuery(db);
    if(query.isActive()){
        query.first();
    }
}


//フラックスto温度変換マトリックス
double ShowImage::gettemperature(double FT1){

    double tmp1=0;

    if(FT1<=0)tmp1=-205;
    else if(FT1<=1e-05){
        if(FT1==1e-05)tmp1=-203;
        else if(FT1<=1e-05)tmp1=-202.9;
        else if(FT1<=1e-05)tmp1=-202.8;
        else if(FT1<=1e-05)tmp1=-202.7;
        else if(FT1<=1e-05)tmp1=-202.6;
        else if(FT1<=1e-05)tmp1=-202.5;
        else if(FT1<=1e-05)tmp1=-202.4;
        else if(FT1<=1e-05)tmp1=-202.3;
        else if(FT1<=1e-05)tmp1=-202.2;
        else if(FT1<=1e-05)tmp1=-202.1;
        else if(FT1<=1e-05)tmp1=-202;
    }
    else if(FT1<=2e-05){
        if(FT1==1e-05)tmp1=-202;
        else if(FT1<=1e-05)tmp1=-201.9;
        else if(FT1<=1e-05)tmp1=-201.8;
        else if(FT1<=1e-05)tmp1=-201.7;
        else if(FT1<=2e-05)tmp1=-201.6;
        else if(FT1<=2e-05)tmp1=-201.5;
        else if(FT1<=2e-05)tmp1=-201.4;
        else if(FT1<=2e-05)tmp1=-201.3;
        else if(FT1<=2e-05)tmp1=-201.2;
        else if(FT1<=2e-05)tmp1=-201.1;
        else if(FT1<=2e-05)tmp1=-201;
    }
    else if(FT1<=2e-05){
        if(FT1==2e-05)tmp1=-201;
        else if(FT1<=2e-05)tmp1=-200.9;
        else if(FT1<=2e-05)tmp1=-200.8;
        else if(FT1<=2e-05)tmp1=-200.7;
        else if(FT1<=2e-05)tmp1=-200.6;
        else if(FT1<=2e-05)tmp1=-200.5;
        else if(FT1<=2e-05)tmp1=-200.4;
        else if(FT1<=2e-05)tmp1=-200.3;
        else if(FT1<=2e-05)tmp1=-200.2;
        else if(FT1<=2e-05)tmp1=-200.1;
        else if(FT1<=2e-05)tmp1=-200;
    }
    else if(FT1<=3e-05){
        if(FT1==2e-05)tmp1=-200;
        else if(FT1<=2e-05)tmp1=-199.9;
        else if(FT1<=2e-05)tmp1=-199.8;
        else if(FT1<=2e-05)tmp1=-199.7;
        else if(FT1<=2e-05)tmp1=-199.6;
        else if(FT1<=3e-05)tmp1=-199.5;
        else if(FT1<=3e-05)tmp1=-199.4;
        else if(FT1<=3e-05)tmp1=-199.3;
        else if(FT1<=3e-05)tmp1=-199.2;
        else if(FT1<=3e-05)tmp1=-199.1;
        else if(FT1<=3e-05)tmp1=-199;
    }
    else if(FT1<=4e-05){
        if(FT1==3e-05)tmp1=-199;
        else if(FT1<=3e-05)tmp1=-198.9;
        else if(FT1<=3e-05)tmp1=-198.8;
        else if(FT1<=3e-05)tmp1=-198.7;
        else if(FT1<=3e-05)tmp1=-198.6;
        else if(FT1<=3e-05)tmp1=-198.5;
        else if(FT1<=3e-05)tmp1=-198.4;
        else if(FT1<=3e-05)tmp1=-198.3;
        else if(FT1<=3e-05)tmp1=-198.2;
        else if(FT1<=4e-05)tmp1=-198.1;
        else if(FT1<=4e-05)tmp1=-198;
    }
    else if(FT1<=4e-05){
        if(FT1==4e-05)tmp1=-198;
        else if(FT1<=4e-05)tmp1=-197.9;
        else if(FT1<=4e-05)tmp1=-197.8;
        else if(FT1<=4e-05)tmp1=-197.7;
        else if(FT1<=4e-05)tmp1=-197.6;
        else if(FT1<=4e-05)tmp1=-197.5;
        else if(FT1<=4e-05)tmp1=-197.4;
        else if(FT1<=4e-05)tmp1=-197.3;
        else if(FT1<=4e-05)tmp1=-197.2;
        else if(FT1<=4e-05)tmp1=-197.1;
        else if(FT1<=5e-05)tmp1=-197;
    }
    else if(FT1<=5e-05){
        if(FT1==5e-05)tmp1=-197;
        else if(FT1<=5e-05)tmp1=-196.9;
        else if(FT1<=5e-05)tmp1=-196.8;
        else if(FT1<=5e-05)tmp1=-196.7;
        else if(FT1<=5e-05)tmp1=-196.6;
        else if(FT1<=5e-05)tmp1=-196.5;
        else if(FT1<=5e-05)tmp1=-196.4;
        else if(FT1<=5e-05)tmp1=-196.3;
        else if(FT1<=5e-05)tmp1=-196.2;
        else if(FT1<=5e-05)tmp1=-196.1;
        else if(FT1<=6e-05)tmp1=-196;
    }
    else if(FT1<=7e-05){
        if(FT1==6e-05)tmp1=-196;
        else if(FT1<=6e-05)tmp1=-195.9;
        else if(FT1<=6e-05)tmp1=-195.8;
        else if(FT1<=6e-05)tmp1=-195.7;
        else if(FT1<=6e-05)tmp1=-195.6;
        else if(FT1<=6e-05)tmp1=-195.5;
        else if(FT1<=6e-05)tmp1=-195.4;
        else if(FT1<=7e-05)tmp1=-195.3;
        else if(FT1<=7e-05)tmp1=-195.2;
        else if(FT1<=7e-05)tmp1=-195.1;
        else if(FT1<=7e-05)tmp1=-195;
    }
    else if(FT1<=8e-05){
        if(FT1==7e-05)tmp1=-195;
        else if(FT1<=7e-05)tmp1=-194.9;
        else if(FT1<=7e-05)tmp1=-194.8;
        else if(FT1<=7e-05)tmp1=-194.7;
        else if(FT1<=8e-05)tmp1=-194.6;
        else if(FT1<=8e-05)tmp1=-194.5;
        else if(FT1<=8e-05)tmp1=-194.4;
        else if(FT1<=8e-05)tmp1=-194.3;
        else if(FT1<=8e-05)tmp1=-194.2;
        else if(FT1<=8e-05)tmp1=-194.1;
        else if(FT1<=9e-05)tmp1=-194;
    }
    else if(FT1<=0.0001){
        if(FT1==9e-05)tmp1=-194;
        else if(FT1<=9e-05)tmp1=-193.9;
        else if(FT1<=9e-05)tmp1=-193.8;
        else if(FT1<=9e-05)tmp1=-193.7;
        else if(FT1<=9e-05)tmp1=-193.6;
        else if(FT1<=9e-05)tmp1=-193.5;
        else if(FT1<=0.0001)tmp1=-193.4;
        else if(FT1<=0.0001)tmp1=-193.3;
        else if(FT1<=0.0001)tmp1=-193.2;
        else if(FT1<=0.0001)tmp1=-193.1;
        else if(FT1<=0.00011)tmp1=-193;
    }
    else if(FT1<=0.00013){
        if(FT1==0.00011)tmp1=-193;
        else if(FT1<=0.00011)tmp1=-192.9;
        else if(FT1<=0.00011)tmp1=-192.8;
        else if(FT1<=0.00011)tmp1=-192.7;
        else if(FT1<=0.00011)tmp1=-192.6;
        else if(FT1<=0.00012)tmp1=-192.5;
        else if(FT1<=0.00012)tmp1=-192.4;
        else if(FT1<=0.00012)tmp1=-192.3;
        else if(FT1<=0.00012)tmp1=-192.2;
        else if(FT1<=0.00013)tmp1=-192.1;
        else if(FT1<=0.00013)tmp1=-192;
    }
    else if(FT1<=0.00015){
        if(FT1==0.00013)tmp1=-192;
        else if(FT1<=0.00013)tmp1=-191.9;
        else if(FT1<=0.00013)tmp1=-191.8;
        else if(FT1<=0.00014)tmp1=-191.7;
        else if(FT1<=0.00014)tmp1=-191.6;
        else if(FT1<=0.00014)tmp1=-191.5;
        else if(FT1<=0.00014)tmp1=-191.4;
        else if(FT1<=0.00015)tmp1=-191.3;
        else if(FT1<=0.00015)tmp1=-191.2;
        else if(FT1<=0.00015)tmp1=-191.1;
        else if(FT1<=0.00016)tmp1=-191;
    }
    else if(FT1<=0.00018){
        if(FT1==0.00016)tmp1=-191;
        else if(FT1<=0.00016)tmp1=-190.9;
        else if(FT1<=0.00016)tmp1=-190.8;
        else if(FT1<=0.00016)tmp1=-190.7;
        else if(FT1<=0.00017)tmp1=-190.6;
        else if(FT1<=0.00017)tmp1=-190.5;
        else if(FT1<=0.00017)tmp1=-190.4;
        else if(FT1<=0.00018)tmp1=-190.3;
        else if(FT1<=0.00018)tmp1=-190.2;
        else if(FT1<=0.00018)tmp1=-190.1;
        else if(FT1<=0.00019)tmp1=-190;
    }
    else if(FT1<=0.00022){
        if(FT1==0.00019)tmp1=-190;
        else if(FT1<=0.00019)tmp1=-189.9;
        else if(FT1<=0.0002)tmp1=-189.8;
        else if(FT1<=0.0002)tmp1=-189.7;
        else if(FT1<=0.0002)tmp1=-189.6;
        else if(FT1<=0.00021)tmp1=-189.5;
        else if(FT1<=0.00021)tmp1=-189.4;
        else if(FT1<=0.00021)tmp1=-189.3;
        else if(FT1<=0.00022)tmp1=-189.2;
        else if(FT1<=0.00022)tmp1=-189.1;
        else if(FT1<=0.00023)tmp1=-189;
    }
    else if(FT1<=0.00027){
        if(FT1==0.00023)tmp1=-189;
        else if(FT1<=0.00023)tmp1=-188.9;
        else if(FT1<=0.00024)tmp1=-188.8;
        else if(FT1<=0.00024)tmp1=-188.7;
        else if(FT1<=0.00024)tmp1=-188.6;
        else if(FT1<=0.00025)tmp1=-188.5;
        else if(FT1<=0.00025)tmp1=-188.4;
        else if(FT1<=0.00026)tmp1=-188.3;
        else if(FT1<=0.00026)tmp1=-188.2;
        else if(FT1<=0.00027)tmp1=-188.1;
        else if(FT1<=0.00027)tmp1=-188;
    }
    else if(FT1<=0.00032){
        if(FT1==0.00027)tmp1=-188;
        else if(FT1<=0.00028)tmp1=-187.9;
        else if(FT1<=0.00028)tmp1=-187.8;
        else if(FT1<=0.00029)tmp1=-187.7;
        else if(FT1<=0.00029)tmp1=-187.6;
        else if(FT1<=0.0003)tmp1=-187.5;
        else if(FT1<=0.0003)tmp1=-187.4;
        else if(FT1<=0.00031)tmp1=-187.3;
        else if(FT1<=0.00031)tmp1=-187.2;
        else if(FT1<=0.00032)tmp1=-187.1;
        else if(FT1<=0.00032)tmp1=-187;
    }
    else if(FT1<=0.00038){
        if(FT1==0.00032)tmp1=-187;
        else if(FT1<=0.00033)tmp1=-186.9;
        else if(FT1<=0.00034)tmp1=-186.8;
        else if(FT1<=0.00034)tmp1=-186.7;
        else if(FT1<=0.00035)tmp1=-186.6;
        else if(FT1<=0.00035)tmp1=-186.5;
        else if(FT1<=0.00036)tmp1=-186.4;
        else if(FT1<=0.00037)tmp1=-186.3;
        else if(FT1<=0.00037)tmp1=-186.2;
        else if(FT1<=0.00038)tmp1=-186.1;
        else if(FT1<=0.00039)tmp1=-186;
    }
    else if(FT1<=0.00045){
        if(FT1==0.00039)tmp1=-186;
        else if(FT1<=0.00039)tmp1=-185.9;
        else if(FT1<=0.0004)tmp1=-185.8;
        else if(FT1<=0.00041)tmp1=-185.7;
        else if(FT1<=0.00041)tmp1=-185.6;
        else if(FT1<=0.00042)tmp1=-185.5;
        else if(FT1<=0.00043)tmp1=-185.4;
        else if(FT1<=0.00044)tmp1=-185.3;
        else if(FT1<=0.00044)tmp1=-185.2;
        else if(FT1<=0.00045)tmp1=-185.1;
        else if(FT1<=0.00046)tmp1=-185;
    }
    else if(FT1<=0.00053){
        if(FT1==0.00046)tmp1=-185;
        else if(FT1<=0.00047)tmp1=-184.9;
        else if(FT1<=0.00047)tmp1=-184.8;
        else if(FT1<=0.00048)tmp1=-184.7;
        else if(FT1<=0.00049)tmp1=-184.6;
        else if(FT1<=0.0005)tmp1=-184.5;
        else if(FT1<=0.00051)tmp1=-184.4;
        else if(FT1<=0.00051)tmp1=-184.3;
        else if(FT1<=0.00052)tmp1=-184.2;
        else if(FT1<=0.00053)tmp1=-184.1;
        else if(FT1<=0.00054)tmp1=-184;
    }
    else if(FT1<=0.00063){
        if(FT1==0.00054)tmp1=-184;
        else if(FT1<=0.00055)tmp1=-183.9;
        else if(FT1<=0.00056)tmp1=-183.8;
        else if(FT1<=0.00057)tmp1=-183.7;
        else if(FT1<=0.00058)tmp1=-183.6;
        else if(FT1<=0.00059)tmp1=-183.5;
        else if(FT1<=0.0006)tmp1=-183.4;
        else if(FT1<=0.00061)tmp1=-183.3;
        else if(FT1<=0.00062)tmp1=-183.2;
        else if(FT1<=0.00063)tmp1=-183.1;
        else if(FT1<=0.00064)tmp1=-183;
    }
    else if(FT1<=0.00073){
        if(FT1==0.00064)tmp1=-183;
        else if(FT1<=0.00065)tmp1=-182.9;
        else if(FT1<=0.00066)tmp1=-182.8;
        else if(FT1<=0.00067)tmp1=-182.7;
        else if(FT1<=0.00068)tmp1=-182.6;
        else if(FT1<=0.00069)tmp1=-182.5;
        else if(FT1<=0.0007)tmp1=-182.4;
        else if(FT1<=0.00071)tmp1=-182.3;
        else if(FT1<=0.00072)tmp1=-182.2;
        else if(FT1<=0.00073)tmp1=-182.1;
        else if(FT1<=0.00075)tmp1=-182;
    }
    else if(FT1<=0.00086){
        if(FT1==0.00075)tmp1=-182;
        else if(FT1<=0.00076)tmp1=-181.9;
        else if(FT1<=0.00077)tmp1=-181.8;
        else if(FT1<=0.00078)tmp1=-181.7;
        else if(FT1<=0.00079)tmp1=-181.6;
        else if(FT1<=0.00081)tmp1=-181.5;
        else if(FT1<=0.00082)tmp1=-181.4;
        else if(FT1<=0.00083)tmp1=-181.3;
        else if(FT1<=0.00085)tmp1=-181.2;
        else if(FT1<=0.00086)tmp1=-181.1;
        else if(FT1<=0.00087)tmp1=-181;
    }
    else if(FT1<=0.001){
        if(FT1==0.00087)tmp1=-181;
        else if(FT1<=0.00089)tmp1=-180.9;
        else if(FT1<=0.0009)tmp1=-180.8;
        else if(FT1<=0.00091)tmp1=-180.7;
        else if(FT1<=0.00093)tmp1=-180.6;
        else if(FT1<=0.00094)tmp1=-180.5;
        else if(FT1<=0.00096)tmp1=-180.4;
        else if(FT1<=0.00097)tmp1=-180.3;
        else if(FT1<=0.00099)tmp1=-180.2;
        else if(FT1<=0.001)tmp1=-180.1;
        else if(FT1<=0.00102)tmp1=-180;
    }
    else if(FT1<=0.00116){
        if(FT1==0.00102)tmp1=-180;
        else if(FT1<=0.00103)tmp1=-179.9;
        else if(FT1<=0.00105)tmp1=-179.8;
        else if(FT1<=0.00106)tmp1=-179.7;
        else if(FT1<=0.00108)tmp1=-179.6;
        else if(FT1<=0.0011)tmp1=-179.5;
        else if(FT1<=0.00111)tmp1=-179.4;
        else if(FT1<=0.00113)tmp1=-179.3;
        else if(FT1<=0.00115)tmp1=-179.2;
        else if(FT1<=0.00116)tmp1=-179.1;
        else if(FT1<=0.00118)tmp1=-179;
    }
    else if(FT1<=0.00135){
        if(FT1==0.00118)tmp1=-179;
        else if(FT1<=0.0012)tmp1=-178.9;
        else if(FT1<=0.00122)tmp1=-178.8;
        else if(FT1<=0.00123)tmp1=-178.7;
        else if(FT1<=0.00125)tmp1=-178.6;
        else if(FT1<=0.00127)tmp1=-178.5;
        else if(FT1<=0.00129)tmp1=-178.4;
        else if(FT1<=0.00131)tmp1=-178.3;
        else if(FT1<=0.00133)tmp1=-178.2;
        else if(FT1<=0.00135)tmp1=-178.1;
        else if(FT1<=0.00137)tmp1=-178;
    }
    else if(FT1<=0.00156){
        if(FT1==0.00137)tmp1=-178;
        else if(FT1<=0.00139)tmp1=-177.9;
        else if(FT1<=0.00141)tmp1=-177.8;
        else if(FT1<=0.00143)tmp1=-177.7;
        else if(FT1<=0.00145)tmp1=-177.6;
        else if(FT1<=0.00147)tmp1=-177.5;
        else if(FT1<=0.00149)tmp1=-177.4;
        else if(FT1<=0.00151)tmp1=-177.3;
        else if(FT1<=0.00153)tmp1=-177.2;
        else if(FT1<=0.00156)tmp1=-177.1;
        else if(FT1<=0.00158)tmp1=-177;
    }
    else if(FT1<=0.00179){
        if(FT1==0.00158)tmp1=-177;
        else if(FT1<=0.0016)tmp1=-176.9;
        else if(FT1<=0.00162)tmp1=-176.8;
        else if(FT1<=0.00165)tmp1=-176.7;
        else if(FT1<=0.00167)tmp1=-176.6;
        else if(FT1<=0.00169)tmp1=-176.5;
        else if(FT1<=0.00172)tmp1=-176.4;
        else if(FT1<=0.00174)tmp1=-176.3;
        else if(FT1<=0.00177)tmp1=-176.2;
        else if(FT1<=0.00179)tmp1=-176.1;
        else if(FT1<=0.00182)tmp1=-176;
    }
    else if(FT1<=0.00206){
        if(FT1==0.00182)tmp1=-176;
        else if(FT1<=0.00184)tmp1=-175.9;
        else if(FT1<=0.00187)tmp1=-175.8;
        else if(FT1<=0.00189)tmp1=-175.7;
        else if(FT1<=0.00192)tmp1=-175.6;
        else if(FT1<=0.00195)tmp1=-175.5;
        else if(FT1<=0.00198)tmp1=-175.4;
        else if(FT1<=0.002)tmp1=-175.3;
        else if(FT1<=0.00203)tmp1=-175.2;
        else if(FT1<=0.00206)tmp1=-175.1;
        else if(FT1<=0.00209)tmp1=-175;
    }
    else if(FT1<=0.00236){
        if(FT1==0.00209)tmp1=-175;
        else if(FT1<=0.00212)tmp1=-174.9;
        else if(FT1<=0.00214)tmp1=-174.8;
        else if(FT1<=0.00217)tmp1=-174.7;
        else if(FT1<=0.0022)tmp1=-174.6;
        else if(FT1<=0.00223)tmp1=-174.5;
        else if(FT1<=0.00226)tmp1=-174.4;
        else if(FT1<=0.0023)tmp1=-174.3;
        else if(FT1<=0.00233)tmp1=-174.2;
        else if(FT1<=0.00236)tmp1=-174.1;
        else if(FT1<=0.00239)tmp1=-174;
    }
    else if(FT1<=0.0027){
        if(FT1==0.00239)tmp1=-174;
        else if(FT1<=0.00242)tmp1=-173.9;
        else if(FT1<=0.00246)tmp1=-173.8;
        else if(FT1<=0.00249)tmp1=-173.7;
        else if(FT1<=0.00252)tmp1=-173.6;
        else if(FT1<=0.00256)tmp1=-173.5;
        else if(FT1<=0.00259)tmp1=-173.4;
        else if(FT1<=0.00262)tmp1=-173.3;
        else if(FT1<=0.00266)tmp1=-173.2;
        else if(FT1<=0.0027)tmp1=-173.1;
        else if(FT1<=0.00273)tmp1=-173;
    }
    else if(FT1<=0.00307){
        if(FT1==0.00273)tmp1=-173;
        else if(FT1<=0.00277)tmp1=-172.9;
        else if(FT1<=0.0028)tmp1=-172.8;
        else if(FT1<=0.00284)tmp1=-172.7;
        else if(FT1<=0.00288)tmp1=-172.6;
        else if(FT1<=0.00292)tmp1=-172.5;
        else if(FT1<=0.00295)tmp1=-172.4;
        else if(FT1<=0.00299)tmp1=-172.3;
        else if(FT1<=0.00303)tmp1=-172.2;
        else if(FT1<=0.00307)tmp1=-172.1;
        else if(FT1<=0.00311)tmp1=-172;
    }
    else if(FT1<=0.00349){
        if(FT1==0.00311)tmp1=-172;
        else if(FT1<=0.00315)tmp1=-171.9;
        else if(FT1<=0.00319)tmp1=-171.8;
        else if(FT1<=0.00323)tmp1=-171.7;
        else if(FT1<=0.00328)tmp1=-171.6;
        else if(FT1<=0.00332)tmp1=-171.5;
        else if(FT1<=0.00336)tmp1=-171.4;
        else if(FT1<=0.00341)tmp1=-171.3;
        else if(FT1<=0.00345)tmp1=-171.2;
        else if(FT1<=0.00349)tmp1=-171.1;
        else if(FT1<=0.00354)tmp1=-171;
    }
    else if(FT1<=0.00396){
        if(FT1==0.00354)tmp1=-171;
        else if(FT1<=0.00358)tmp1=-170.9;
        else if(FT1<=0.00363)tmp1=-170.8;
        else if(FT1<=0.00367)tmp1=-170.7;
        else if(FT1<=0.00372)tmp1=-170.6;
        else if(FT1<=0.00377)tmp1=-170.5;
        else if(FT1<=0.00382)tmp1=-170.4;
        else if(FT1<=0.00386)tmp1=-170.3;
        else if(FT1<=0.00391)tmp1=-170.2;
        else if(FT1<=0.00396)tmp1=-170.1;
        else if(FT1<=0.00401)tmp1=-170;
    }
    else if(FT1<=0.00448){
        if(FT1==0.00401)tmp1=-170;
        else if(FT1<=0.00406)tmp1=-169.9;
        else if(FT1<=0.00411)tmp1=-169.8;
        else if(FT1<=0.00416)tmp1=-169.7;
        else if(FT1<=0.00422)tmp1=-169.6;
        else if(FT1<=0.00427)tmp1=-169.5;
        else if(FT1<=0.00432)tmp1=-169.4;
        else if(FT1<=0.00438)tmp1=-169.3;
        else if(FT1<=0.00443)tmp1=-169.2;
        else if(FT1<=0.00448)tmp1=-169.1;
        else if(FT1<=0.00454)tmp1=-169;
    }
    else if(FT1<=0.00506){
        if(FT1==0.00454)tmp1=-169;
        else if(FT1<=0.0046)tmp1=-168.9;
        else if(FT1<=0.00465)tmp1=-168.8;
        else if(FT1<=0.00471)tmp1=-168.7;
        else if(FT1<=0.00477)tmp1=-168.6;
        else if(FT1<=0.00483)tmp1=-168.5;
        else if(FT1<=0.00488)tmp1=-168.4;
        else if(FT1<=0.00494)tmp1=-168.3;
        else if(FT1<=0.005)tmp1=-168.2;
        else if(FT1<=0.00506)tmp1=-168.1;
        else if(FT1<=0.00513)tmp1=-168;
    }
    else if(FT1<=0.00571){
        if(FT1==0.00513)tmp1=-168;
        else if(FT1<=0.00519)tmp1=-167.9;
        else if(FT1<=0.00525)tmp1=-167.8;
        else if(FT1<=0.00531)tmp1=-167.7;
        else if(FT1<=0.00538)tmp1=-167.6;
        else if(FT1<=0.00544)tmp1=-167.5;
        else if(FT1<=0.00551)tmp1=-167.4;
        else if(FT1<=0.00557)tmp1=-167.3;
        else if(FT1<=0.00564)tmp1=-167.2;
        else if(FT1<=0.00571)tmp1=-167.1;
        else if(FT1<=0.00577)tmp1=-167;
    }
    else if(FT1<=0.00642){
        if(FT1==0.00577)tmp1=-167;
        else if(FT1<=0.00584)tmp1=-166.9;
        else if(FT1<=0.00591)tmp1=-166.8;
        else if(FT1<=0.00598)tmp1=-166.7;
        else if(FT1<=0.00605)tmp1=-166.6;
        else if(FT1<=0.00612)tmp1=-166.5;
        else if(FT1<=0.0062)tmp1=-166.4;
        else if(FT1<=0.00627)tmp1=-166.3;
        else if(FT1<=0.00634)tmp1=-166.2;
        else if(FT1<=0.00642)tmp1=-166.1;
        else if(FT1<=0.00649)tmp1=-166;
    }
    else if(FT1<=0.0072){
        if(FT1==0.00649)tmp1=-166;
        else if(FT1<=0.00657)tmp1=-165.9;
        else if(FT1<=0.00664)tmp1=-165.8;
        else if(FT1<=0.00672)tmp1=-165.7;
        else if(FT1<=0.0068)tmp1=-165.6;
        else if(FT1<=0.00688)tmp1=-165.5;
        else if(FT1<=0.00696)tmp1=-165.4;
        else if(FT1<=0.00704)tmp1=-165.3;
        else if(FT1<=0.00712)tmp1=-165.2;
        else if(FT1<=0.0072)tmp1=-165.1;
        else if(FT1<=0.00728)tmp1=-165;
    }
    else if(FT1<=0.00806){
        if(FT1==0.00728)tmp1=-165;
        else if(FT1<=0.00737)tmp1=-164.9;
        else if(FT1<=0.00745)tmp1=-164.8;
        else if(FT1<=0.00754)tmp1=-164.7;
        else if(FT1<=0.00762)tmp1=-164.6;
        else if(FT1<=0.00771)tmp1=-164.5;
        else if(FT1<=0.0078)tmp1=-164.4;
        else if(FT1<=0.00788)tmp1=-164.3;
        else if(FT1<=0.00797)tmp1=-164.2;
        else if(FT1<=0.00806)tmp1=-164.1;
        else if(FT1<=0.00815)tmp1=-164;
    }
    else if(FT1<=0.00901){
        if(FT1==0.00815)tmp1=-164;
        else if(FT1<=0.00825)tmp1=-163.9;
        else if(FT1<=0.00834)tmp1=-163.8;
        else if(FT1<=0.00843)tmp1=-163.7;
        else if(FT1<=0.00853)tmp1=-163.6;
        else if(FT1<=0.00862)tmp1=-163.5;
        else if(FT1<=0.00872)tmp1=-163.4;
        else if(FT1<=0.00882)tmp1=-163.3;
        else if(FT1<=0.00891)tmp1=-163.2;
        else if(FT1<=0.00901)tmp1=-163.1;
        else if(FT1<=0.00911)tmp1=-163;
    }
    else if(FT1<=0.01005){
        if(FT1==0.00911)tmp1=-163;
        else if(FT1<=0.00921)tmp1=-162.9;
        else if(FT1<=0.00932)tmp1=-162.8;
        else if(FT1<=0.00942)tmp1=-162.7;
        else if(FT1<=0.00952)tmp1=-162.6;
        else if(FT1<=0.00963)tmp1=-162.5;
        else if(FT1<=0.00973)tmp1=-162.4;
        else if(FT1<=0.00984)tmp1=-162.3;
        else if(FT1<=0.00995)tmp1=-162.2;
        else if(FT1<=0.01005)tmp1=-162.1;
        else if(FT1<=0.01016)tmp1=-162;
    }
    else if(FT1<=0.0112){
        if(FT1==0.01016)tmp1=-162;
        else if(FT1<=0.01027)tmp1=-161.9;
        else if(FT1<=0.01039)tmp1=-161.8;
        else if(FT1<=0.0105)tmp1=-161.7;
        else if(FT1<=0.01061)tmp1=-161.6;
        else if(FT1<=0.01073)tmp1=-161.5;
        else if(FT1<=0.01084)tmp1=-161.4;
        else if(FT1<=0.01096)tmp1=-161.3;
        else if(FT1<=0.01108)tmp1=-161.2;
        else if(FT1<=0.0112)tmp1=-161.1;
        else if(FT1<=0.01132)tmp1=-161;
    }
    else if(FT1<=0.01244){
        if(FT1==0.01132)tmp1=-161;
        else if(FT1<=0.01144)tmp1=-160.9;
        else if(FT1<=0.01156)tmp1=-160.8;
        else if(FT1<=0.01168)tmp1=-160.7;
        else if(FT1<=0.01181)tmp1=-160.6;
        else if(FT1<=0.01193)tmp1=-160.5;
        else if(FT1<=0.01206)tmp1=-160.4;
        else if(FT1<=0.01219)tmp1=-160.3;
        else if(FT1<=0.01231)tmp1=-160.2;
        else if(FT1<=0.01244)tmp1=-160.1;
        else if(FT1<=0.01258)tmp1=-160;
    }
    else if(FT1<=0.01381){
        if(FT1==0.01258)tmp1=-160;
        else if(FT1<=0.01271)tmp1=-159.9;
        else if(FT1<=0.01284)tmp1=-159.8;
        else if(FT1<=0.01298)tmp1=-159.7;
        else if(FT1<=0.01311)tmp1=-159.6;
        else if(FT1<=0.01325)tmp1=-159.5;
        else if(FT1<=0.01339)tmp1=-159.4;
        else if(FT1<=0.01353)tmp1=-159.3;
        else if(FT1<=0.01367)tmp1=-159.2;
        else if(FT1<=0.01381)tmp1=-159.1;
        else if(FT1<=0.01395)tmp1=-159;
    }
    else if(FT1<=0.01529){
        if(FT1==0.01395)tmp1=-159;
        else if(FT1<=0.0141)tmp1=-158.9;
        else if(FT1<=0.01424)tmp1=-158.8;
        else if(FT1<=0.01439)tmp1=-158.7;
        else if(FT1<=0.01454)tmp1=-158.6;
        else if(FT1<=0.01468)tmp1=-158.5;
        else if(FT1<=0.01484)tmp1=-158.4;
        else if(FT1<=0.01499)tmp1=-158.3;
        else if(FT1<=0.01514)tmp1=-158.2;
        else if(FT1<=0.01529)tmp1=-158.1;
        else if(FT1<=0.01545)tmp1=-158;
    }
    else if(FT1<=0.01691){
        if(FT1==0.01545)tmp1=-158;
        else if(FT1<=0.01561)tmp1=-157.9;
        else if(FT1<=0.01577)tmp1=-157.8;
        else if(FT1<=0.01593)tmp1=-157.7;
        else if(FT1<=0.01609)tmp1=-157.6;
        else if(FT1<=0.01625)tmp1=-157.5;
        else if(FT1<=0.01641)tmp1=-157.4;
        else if(FT1<=0.01658)tmp1=-157.3;
        else if(FT1<=0.01675)tmp1=-157.2;
        else if(FT1<=0.01691)tmp1=-157.1;
        else if(FT1<=0.01708)tmp1=-157;
    }
    else if(FT1<=0.01867){
        if(FT1==0.01708)tmp1=-157;
        else if(FT1<=0.01725)tmp1=-156.9;
        else if(FT1<=0.01743)tmp1=-156.8;
        else if(FT1<=0.0176)tmp1=-156.7;
        else if(FT1<=0.01777)tmp1=-156.6;
        else if(FT1<=0.01795)tmp1=-156.5;
        else if(FT1<=0.01813)tmp1=-156.4;
        else if(FT1<=0.01831)tmp1=-156.3;
        else if(FT1<=0.01849)tmp1=-156.2;
        else if(FT1<=0.01867)tmp1=-156.1;
        else if(FT1<=0.01886)tmp1=-156;
    }
    else if(FT1<=0.02058){
        if(FT1==0.01886)tmp1=-156;
        else if(FT1<=0.01904)tmp1=-155.9;
        else if(FT1<=0.01923)tmp1=-155.8;
        else if(FT1<=0.01942)tmp1=-155.7;
        else if(FT1<=0.01961)tmp1=-155.6;
        else if(FT1<=0.0198)tmp1=-155.5;
        else if(FT1<=0.01999)tmp1=-155.4;
        else if(FT1<=0.02019)tmp1=-155.3;
        else if(FT1<=0.02038)tmp1=-155.2;
        else if(FT1<=0.02058)tmp1=-155.1;
        else if(FT1<=0.02078)tmp1=-155;
    }
    else if(FT1<=0.02265){
        if(FT1==0.02078)tmp1=-155;
        else if(FT1<=0.02098)tmp1=-154.9;
        else if(FT1<=0.02119)tmp1=-154.8;
        else if(FT1<=0.02139)tmp1=-154.7;
        else if(FT1<=0.0216)tmp1=-154.6;
        else if(FT1<=0.0218)tmp1=-154.5;
        else if(FT1<=0.02201)tmp1=-154.4;
        else if(FT1<=0.02222)tmp1=-154.3;
        else if(FT1<=0.02244)tmp1=-154.2;
        else if(FT1<=0.02265)tmp1=-154.1;
        else if(FT1<=0.02287)tmp1=-154;
    }
    else if(FT1<=0.02489){
        if(FT1==0.02287)tmp1=-154;
        else if(FT1<=0.02309)tmp1=-153.9;
        else if(FT1<=0.02331)tmp1=-153.8;
        else if(FT1<=0.02353)tmp1=-153.7;
        else if(FT1<=0.02375)tmp1=-153.6;
        else if(FT1<=0.02397)tmp1=-153.5;
        else if(FT1<=0.0242)tmp1=-153.4;
        else if(FT1<=0.02443)tmp1=-153.3;
        else if(FT1<=0.02466)tmp1=-153.2;
        else if(FT1<=0.02489)tmp1=-153.1;
        else if(FT1<=0.02513)tmp1=-153;
    }
    else if(FT1<=0.02731){
        if(FT1==0.02513)tmp1=-153;
        else if(FT1<=0.02536)tmp1=-152.9;
        else if(FT1<=0.0256)tmp1=-152.8;
        else if(FT1<=0.02584)tmp1=-152.7;
        else if(FT1<=0.02608)tmp1=-152.6;
        else if(FT1<=0.02632)tmp1=-152.5;
        else if(FT1<=0.02657)tmp1=-152.4;
        else if(FT1<=0.02681)tmp1=-152.3;
        else if(FT1<=0.02706)tmp1=-152.2;
        else if(FT1<=0.02731)tmp1=-152.1;
        else if(FT1<=0.02757)tmp1=-152;
    }
    else if(FT1<=0.02993){
        if(FT1==0.02757)tmp1=-152;
        else if(FT1<=0.02782)tmp1=-151.9;
        else if(FT1<=0.02808)tmp1=-151.8;
        else if(FT1<=0.02834)tmp1=-151.7;
        else if(FT1<=0.0286)tmp1=-151.6;
        else if(FT1<=0.02886)tmp1=-151.5;
        else if(FT1<=0.02912)tmp1=-151.4;
        else if(FT1<=0.02939)tmp1=-151.3;
        else if(FT1<=0.02966)tmp1=-151.2;
        else if(FT1<=0.02993)tmp1=-151.1;
        else if(FT1<=0.0302)tmp1=-151;
    }
    else if(FT1<=0.03275){
        if(FT1==0.0302)tmp1=-151;
        else if(FT1<=0.03047)tmp1=-150.9;
        else if(FT1<=0.03075)tmp1=-150.8;
        else if(FT1<=0.03103)tmp1=-150.7;
        else if(FT1<=0.03131)tmp1=-150.6;
        else if(FT1<=0.03159)tmp1=-150.5;
        else if(FT1<=0.03188)tmp1=-150.4;
        else if(FT1<=0.03216)tmp1=-150.3;
        else if(FT1<=0.03245)tmp1=-150.2;
        else if(FT1<=0.03275)tmp1=-150.1;
        else if(FT1<=0.03304)tmp1=-150;
    }
    else if(FT1<=0.03578){
        if(FT1==0.03304)tmp1=-150;
        else if(FT1<=0.03333)tmp1=-149.9;
        else if(FT1<=0.03363)tmp1=-149.8;
        else if(FT1<=0.03393)tmp1=-149.7;
        else if(FT1<=0.03423)tmp1=-149.6;
        else if(FT1<=0.03454)tmp1=-149.5;
        else if(FT1<=0.03485)tmp1=-149.4;
        else if(FT1<=0.03515)tmp1=-149.3;
        else if(FT1<=0.03547)tmp1=-149.2;
        else if(FT1<=0.03578)tmp1=-149.1;
        else if(FT1<=0.03609)tmp1=-149;
    }
    else if(FT1<=0.03904){
        if(FT1==0.03609)tmp1=-149;
        else if(FT1<=0.03641)tmp1=-148.9;
        else if(FT1<=0.03673)tmp1=-148.8;
        else if(FT1<=0.03706)tmp1=-148.7;
        else if(FT1<=0.03738)tmp1=-148.6;
        else if(FT1<=0.03771)tmp1=-148.5;
        else if(FT1<=0.03804)tmp1=-148.4;
        else if(FT1<=0.03837)tmp1=-148.3;
        else if(FT1<=0.0387)tmp1=-148.2;
        else if(FT1<=0.03904)tmp1=-148.1;
        else if(FT1<=0.03938)tmp1=-148;
    }
    else if(FT1<=0.04255){
        if(FT1==0.03938)tmp1=-148;
        else if(FT1<=0.03972)tmp1=-147.9;
        else if(FT1<=0.04007)tmp1=-147.8;
        else if(FT1<=0.04041)tmp1=-147.7;
        else if(FT1<=0.04076)tmp1=-147.6;
        else if(FT1<=0.04111)tmp1=-147.5;
        else if(FT1<=0.04147)tmp1=-147.4;
        else if(FT1<=0.04182)tmp1=-147.3;
        else if(FT1<=0.04218)tmp1=-147.2;
        else if(FT1<=0.04255)tmp1=-147.1;
        else if(FT1<=0.04291)tmp1=-147;
    }
    else if(FT1<=0.0463){
        if(FT1==0.04291)tmp1=-147;
        else if(FT1<=0.04328)tmp1=-146.9;
        else if(FT1<=0.04365)tmp1=-146.8;
        else if(FT1<=0.04402)tmp1=-146.7;
        else if(FT1<=0.04439)tmp1=-146.6;
        else if(FT1<=0.04477)tmp1=-146.5;
        else if(FT1<=0.04515)tmp1=-146.4;
        else if(FT1<=0.04553)tmp1=-146.3;
        else if(FT1<=0.04592)tmp1=-146.2;
        else if(FT1<=0.0463)tmp1=-146.1;
        else if(FT1<=0.0467)tmp1=-146;
    }
    else if(FT1<=0.05033){
        if(FT1==0.0467)tmp1=-146;
        else if(FT1<=0.04709)tmp1=-145.9;
        else if(FT1<=0.04748)tmp1=-145.8;
        else if(FT1<=0.04788)tmp1=-145.7;
        else if(FT1<=0.04828)tmp1=-145.6;
        else if(FT1<=0.04869)tmp1=-145.5;
        else if(FT1<=0.0491)tmp1=-145.4;
        else if(FT1<=0.04951)tmp1=-145.3;
        else if(FT1<=0.04992)tmp1=-145.2;
        else if(FT1<=0.05033)tmp1=-145.1;
        else if(FT1<=0.05075)tmp1=-145;
    }
    else if(FT1<=0.05464){
        if(FT1==0.05075)tmp1=-145;
        else if(FT1<=0.05117)tmp1=-144.9;
        else if(FT1<=0.0516)tmp1=-144.8;
        else if(FT1<=0.05202)tmp1=-144.7;
        else if(FT1<=0.05245)tmp1=-144.6;
        else if(FT1<=0.05289)tmp1=-144.5;
        else if(FT1<=0.05332)tmp1=-144.4;
        else if(FT1<=0.05376)tmp1=-144.3;
        else if(FT1<=0.0542)tmp1=-144.2;
        else if(FT1<=0.05464)tmp1=-144.1;
        else if(FT1<=0.05509)tmp1=-144;
    }
    else if(FT1<=0.05925){
        if(FT1==0.05509)tmp1=-144;
        else if(FT1<=0.05554)tmp1=-143.9;
        else if(FT1<=0.056)tmp1=-143.8;
        else if(FT1<=0.05645)tmp1=-143.7;
        else if(FT1<=0.05691)tmp1=-143.6;
        else if(FT1<=0.05737)tmp1=-143.5;
        else if(FT1<=0.05784)tmp1=-143.4;
        else if(FT1<=0.05831)tmp1=-143.3;
        else if(FT1<=0.05878)tmp1=-143.2;
        else if(FT1<=0.05925)tmp1=-143.1;
        else if(FT1<=0.05973)tmp1=-143;
    }
    else if(FT1<=0.06418){
        if(FT1==0.05973)tmp1=-143;
        else if(FT1<=0.06021)tmp1=-142.9;
        else if(FT1<=0.0607)tmp1=-142.8;
        else if(FT1<=0.06119)tmp1=-142.7;
        else if(FT1<=0.06168)tmp1=-142.6;
        else if(FT1<=0.06217)tmp1=-142.5;
        else if(FT1<=0.06267)tmp1=-142.4;
        else if(FT1<=0.06317)tmp1=-142.3;
        else if(FT1<=0.06367)tmp1=-142.2;
        else if(FT1<=0.06418)tmp1=-142.1;
        else if(FT1<=0.06469)tmp1=-142;
    }
    else if(FT1<=0.06943){
        if(FT1==0.06469)tmp1=-142;
        else if(FT1<=0.0652)tmp1=-141.9;
        else if(FT1<=0.06572)tmp1=-141.8;
        else if(FT1<=0.06624)tmp1=-141.7;
        else if(FT1<=0.06676)tmp1=-141.6;
        else if(FT1<=0.06729)tmp1=-141.5;
        else if(FT1<=0.06782)tmp1=-141.4;
        else if(FT1<=0.06835)tmp1=-141.3;
        else if(FT1<=0.06889)tmp1=-141.2;
        else if(FT1<=0.06943)tmp1=-141.1;
        else if(FT1<=0.06998)tmp1=-141;
    }
    else if(FT1<=0.07503){
        if(FT1==0.06998)tmp1=-141;
        else if(FT1<=0.07052)tmp1=-140.9;
        else if(FT1<=0.07107)tmp1=-140.8;
        else if(FT1<=0.07163)tmp1=-140.7;
        else if(FT1<=0.07219)tmp1=-140.6;
        else if(FT1<=0.07275)tmp1=-140.5;
        else if(FT1<=0.07331)tmp1=-140.4;
        else if(FT1<=0.07388)tmp1=-140.3;
        else if(FT1<=0.07445)tmp1=-140.2;
        else if(FT1<=0.07503)tmp1=-140.1;
        else if(FT1<=0.07561)tmp1=-140;
    }
    else if(FT1<=0.08099){
        if(FT1==0.07561)tmp1=-140;
        else if(FT1<=0.07619)tmp1=-139.9;
        else if(FT1<=0.07678)tmp1=-139.8;
        else if(FT1<=0.07737)tmp1=-139.7;
        else if(FT1<=0.07797)tmp1=-139.6;
        else if(FT1<=0.07856)tmp1=-139.5;
        else if(FT1<=0.07916)tmp1=-139.4;
        else if(FT1<=0.07977)tmp1=-139.3;
        else if(FT1<=0.08038)tmp1=-139.2;
        else if(FT1<=0.08099)tmp1=-139.1;
        else if(FT1<=0.08161)tmp1=-139;
    }
    else if(FT1<=0.08733){
        if(FT1==0.08161)tmp1=-139;
        else if(FT1<=0.08223)tmp1=-138.9;
        else if(FT1<=0.08285)tmp1=-138.8;
        else if(FT1<=0.08348)tmp1=-138.7;
        else if(FT1<=0.08411)tmp1=-138.6;
        else if(FT1<=0.08475)tmp1=-138.5;
        else if(FT1<=0.08539)tmp1=-138.4;
        else if(FT1<=0.08603)tmp1=-138.3;
        else if(FT1<=0.08668)tmp1=-138.2;
        else if(FT1<=0.08733)tmp1=-138.1;
        else if(FT1<=0.08799)tmp1=-138;
    }
    else if(FT1<=0.09407){
        if(FT1==0.08799)tmp1=-138;
        else if(FT1<=0.08865)tmp1=-137.9;
        else if(FT1<=0.08931)tmp1=-137.8;
        else if(FT1<=0.08998)tmp1=-137.7;
        else if(FT1<=0.09065)tmp1=-137.6;
        else if(FT1<=0.09133)tmp1=-137.5;
        else if(FT1<=0.09201)tmp1=-137.4;
        else if(FT1<=0.09269)tmp1=-137.3;
        else if(FT1<=0.09338)tmp1=-137.2;
        else if(FT1<=0.09407)tmp1=-137.1;
        else if(FT1<=0.09477)tmp1=-137;
    }
    else if(FT1<=0.10123){
        if(FT1==0.09477)tmp1=-137;
        else if(FT1<=0.09547)tmp1=-136.9;
        else if(FT1<=0.09618)tmp1=-136.8;
        else if(FT1<=0.09688)tmp1=-136.7;
        else if(FT1<=0.0976)tmp1=-136.6;
        else if(FT1<=0.09832)tmp1=-136.5;
        else if(FT1<=0.09904)tmp1=-136.4;
        else if(FT1<=0.09976)tmp1=-136.3;
        else if(FT1<=0.10049)tmp1=-136.2;
        else if(FT1<=0.10123)tmp1=-136.1;
        else if(FT1<=0.10197)tmp1=-136;
    }
    else if(FT1<=0.10882){
        if(FT1==0.10197)tmp1=-136;
        else if(FT1<=0.10271)tmp1=-135.9;
        else if(FT1<=0.10346)tmp1=-135.8;
        else if(FT1<=0.10421)tmp1=-135.7;
        else if(FT1<=0.10497)tmp1=-135.6;
        else if(FT1<=0.10573)tmp1=-135.5;
        else if(FT1<=0.10649)tmp1=-135.4;
        else if(FT1<=0.10726)tmp1=-135.3;
        else if(FT1<=0.10804)tmp1=-135.2;
        else if(FT1<=0.10882)tmp1=-135.1;
        else if(FT1<=0.1096)tmp1=-135;
    }
    else if(FT1<=0.11686){
        if(FT1==0.1096)tmp1=-135;
        else if(FT1<=0.11039)tmp1=-134.9;
        else if(FT1<=0.11118)tmp1=-134.8;
        else if(FT1<=0.11198)tmp1=-134.7;
        else if(FT1<=0.11278)tmp1=-134.6;
        else if(FT1<=0.11359)tmp1=-134.5;
        else if(FT1<=0.1144)tmp1=-134.4;
        else if(FT1<=0.11521)tmp1=-134.3;
        else if(FT1<=0.11604)tmp1=-134.2;
        else if(FT1<=0.11686)tmp1=-134.1;
        else if(FT1<=0.11769)tmp1=-134;
    }
    else if(FT1<=0.12538){
        if(FT1==0.11769)tmp1=-134;
        else if(FT1<=0.11853)tmp1=-133.9;
        else if(FT1<=0.11937)tmp1=-133.8;
        else if(FT1<=0.12021)tmp1=-133.7;
        else if(FT1<=0.12106)tmp1=-133.6;
        else if(FT1<=0.12191)tmp1=-133.5;
        else if(FT1<=0.12277)tmp1=-133.4;
        else if(FT1<=0.12364)tmp1=-133.3;
        else if(FT1<=0.1245)tmp1=-133.2;
        else if(FT1<=0.12538)tmp1=-133.1;
        else if(FT1<=0.12626)tmp1=-133;
    }
    else if(FT1<=0.13439){
        if(FT1==0.12626)tmp1=-133;
        else if(FT1<=0.12714)tmp1=-132.9;
        else if(FT1<=0.12803)tmp1=-132.8;
        else if(FT1<=0.12892)tmp1=-132.7;
        else if(FT1<=0.12982)tmp1=-132.6;
        else if(FT1<=0.13072)tmp1=-132.5;
        else if(FT1<=0.13163)tmp1=-132.4;
        else if(FT1<=0.13254)tmp1=-132.3;
        else if(FT1<=0.13346)tmp1=-132.2;
        else if(FT1<=0.13439)tmp1=-132.1;
        else if(FT1<=0.13532)tmp1=-132;
    }
    else if(FT1<=0.14391){
        if(FT1==0.13532)tmp1=-132;
        else if(FT1<=0.13625)tmp1=-131.9;
        else if(FT1<=0.13719)tmp1=-131.8;
        else if(FT1<=0.13813)tmp1=-131.7;
        else if(FT1<=0.13908)tmp1=-131.6;
        else if(FT1<=0.14004)tmp1=-131.5;
        else if(FT1<=0.141)tmp1=-131.4;
        else if(FT1<=0.14196)tmp1=-131.3;
        else if(FT1<=0.14294)tmp1=-131.2;
        else if(FT1<=0.14391)tmp1=-131.1;
        else if(FT1<=0.14489)tmp1=-131;
    }
    else if(FT1<=0.15397){
        if(FT1==0.14489)tmp1=-131;
        else if(FT1<=0.14588)tmp1=-130.9;
        else if(FT1<=0.14687)tmp1=-130.8;
        else if(FT1<=0.14787)tmp1=-130.7;
        else if(FT1<=0.14887)tmp1=-130.6;
        else if(FT1<=0.14988)tmp1=-130.5;
        else if(FT1<=0.1509)tmp1=-130.4;
        else if(FT1<=0.15191)tmp1=-130.3;
        else if(FT1<=0.15294)tmp1=-130.2;
        else if(FT1<=0.15397)tmp1=-130.1;
        else if(FT1<=0.15501)tmp1=-130;
    }
    else if(FT1<=0.16459){
        if(FT1==0.15501)tmp1=-130;
        else if(FT1<=0.15605)tmp1=-129.9;
        else if(FT1<=0.1571)tmp1=-129.8;
        else if(FT1<=0.15815)tmp1=-129.7;
        else if(FT1<=0.15921)tmp1=-129.6;
        else if(FT1<=0.16027)tmp1=-129.5;
        else if(FT1<=0.16134)tmp1=-129.4;
        else if(FT1<=0.16242)tmp1=-129.3;
        else if(FT1<=0.1635)tmp1=-129.2;
        else if(FT1<=0.16459)tmp1=-129.1;
        else if(FT1<=0.16568)tmp1=-129;
    }
    else if(FT1<=0.17578){
        if(FT1==0.16568)tmp1=-129;
        else if(FT1<=0.16678)tmp1=-128.9;
        else if(FT1<=0.16788)tmp1=-128.8;
        else if(FT1<=0.16899)tmp1=-128.7;
        else if(FT1<=0.17011)tmp1=-128.6;
        else if(FT1<=0.17123)tmp1=-128.5;
        else if(FT1<=0.17236)tmp1=-128.4;
        else if(FT1<=0.17349)tmp1=-128.3;
        else if(FT1<=0.17463)tmp1=-128.2;
        else if(FT1<=0.17578)tmp1=-128.1;
        else if(FT1<=0.17693)tmp1=-128;
    }
    else if(FT1<=0.18757){
        if(FT1==0.17693)tmp1=-128;
        else if(FT1<=0.17809)tmp1=-127.9;
        else if(FT1<=0.17925)tmp1=-127.8;
        else if(FT1<=0.18042)tmp1=-127.7;
        else if(FT1<=0.1816)tmp1=-127.6;
        else if(FT1<=0.18278)tmp1=-127.5;
        else if(FT1<=0.18397)tmp1=-127.4;
        else if(FT1<=0.18517)tmp1=-127.3;
        else if(FT1<=0.18637)tmp1=-127.2;
        else if(FT1<=0.18757)tmp1=-127.1;
        else if(FT1<=0.18879)tmp1=-127;
    }
    else if(FT1<=0.19999){
        if(FT1==0.18879)tmp1=-127;
        else if(FT1<=0.19001)tmp1=-126.9;
        else if(FT1<=0.19123)tmp1=-126.8;
        else if(FT1<=0.19247)tmp1=-126.7;
        else if(FT1<=0.1937)tmp1=-126.6;
        else if(FT1<=0.19495)tmp1=-126.5;
        else if(FT1<=0.1962)tmp1=-126.4;
        else if(FT1<=0.19746)tmp1=-126.3;
        else if(FT1<=0.19872)tmp1=-126.2;
        else if(FT1<=0.19999)tmp1=-126.1;
        else if(FT1<=0.20127)tmp1=-126;
    }
    else if(FT1<=0.21306){
        if(FT1==0.20127)tmp1=-126;
        else if(FT1<=0.20255)tmp1=-125.9;
        else if(FT1<=0.20384)tmp1=-125.8;
        else if(FT1<=0.20514)tmp1=-125.7;
        else if(FT1<=0.20644)tmp1=-125.6;
        else if(FT1<=0.20775)tmp1=-125.5;
        else if(FT1<=0.20907)tmp1=-125.4;
        else if(FT1<=0.21039)tmp1=-125.3;
        else if(FT1<=0.21172)tmp1=-125.2;
        else if(FT1<=0.21306)tmp1=-125.1;
        else if(FT1<=0.2144)tmp1=-125;
    }
    else if(FT1<=0.22679){
        if(FT1==0.2144)tmp1=-125;
        else if(FT1<=0.21575)tmp1=-124.9;
        else if(FT1<=0.21711)tmp1=-124.8;
        else if(FT1<=0.21847)tmp1=-124.7;
        else if(FT1<=0.21984)tmp1=-124.6;
        else if(FT1<=0.22122)tmp1=-124.5;
        else if(FT1<=0.2226)tmp1=-124.4;
        else if(FT1<=0.22399)tmp1=-124.3;
        else if(FT1<=0.22539)tmp1=-124.2;
        else if(FT1<=0.22679)tmp1=-124.1;
        else if(FT1<=0.22821)tmp1=-124;
    }
    else if(FT1<=0.24123){
        if(FT1==0.22821)tmp1=-124;
        else if(FT1<=0.22962)tmp1=-123.9;
        else if(FT1<=0.23105)tmp1=-123.8;
        else if(FT1<=0.23248)tmp1=-123.7;
        else if(FT1<=0.23392)tmp1=-123.6;
        else if(FT1<=0.23537)tmp1=-123.5;
        else if(FT1<=0.23682)tmp1=-123.4;
        else if(FT1<=0.23828)tmp1=-123.3;
        else if(FT1<=0.23975)tmp1=-123.2;
        else if(FT1<=0.24123)tmp1=-123.1;
        else if(FT1<=0.24271)tmp1=-123;
    }
    else if(FT1<=0.25638){
        if(FT1==0.24271)tmp1=-123;
        else if(FT1<=0.2442)tmp1=-122.9;
        else if(FT1<=0.24569)tmp1=-122.8;
        else if(FT1<=0.2472)tmp1=-122.7;
        else if(FT1<=0.24871)tmp1=-122.6;
        else if(FT1<=0.25023)tmp1=-122.5;
        else if(FT1<=0.25175)tmp1=-122.4;
        else if(FT1<=0.25329)tmp1=-122.3;
        else if(FT1<=0.25483)tmp1=-122.2;
        else if(FT1<=0.25638)tmp1=-122.1;
        else if(FT1<=0.25793)tmp1=-122;
    }
    else if(FT1<=0.27227){
        if(FT1==0.25793)tmp1=-122;
        else if(FT1<=0.25949)tmp1=-121.9;
        else if(FT1<=0.26106)tmp1=-121.8;
        else if(FT1<=0.26264)tmp1=-121.7;
        else if(FT1<=0.26423)tmp1=-121.6;
        else if(FT1<=0.26582)tmp1=-121.5;
        else if(FT1<=0.26742)tmp1=-121.4;
        else if(FT1<=0.26903)tmp1=-121.3;
        else if(FT1<=0.27065)tmp1=-121.2;
        else if(FT1<=0.27227)tmp1=-121.1;
        else if(FT1<=0.2739)tmp1=-121;
    }
    else if(FT1<=0.28893){
        if(FT1==0.2739)tmp1=-121;
        else if(FT1<=0.27554)tmp1=-120.9;
        else if(FT1<=0.27719)tmp1=-120.8;
        else if(FT1<=0.27884)tmp1=-120.7;
        else if(FT1<=0.2805)tmp1=-120.6;
        else if(FT1<=0.28217)tmp1=-120.5;
        else if(FT1<=0.28385)tmp1=-120.4;
        else if(FT1<=0.28554)tmp1=-120.3;
        else if(FT1<=0.28723)tmp1=-120.2;
        else if(FT1<=0.28893)tmp1=-120.1;
        else if(FT1<=0.29064)tmp1=-120;
    }
    else if(FT1<=0.30639){
        if(FT1==0.29064)tmp1=-120;
        else if(FT1<=0.29236)tmp1=-119.9;
        else if(FT1<=0.29409)tmp1=-119.8;
        else if(FT1<=0.29582)tmp1=-119.7;
        else if(FT1<=0.29756)tmp1=-119.6;
        else if(FT1<=0.29931)tmp1=-119.5;
        else if(FT1<=0.30107)tmp1=-119.4;
        else if(FT1<=0.30284)tmp1=-119.3;
        else if(FT1<=0.30461)tmp1=-119.2;
        else if(FT1<=0.30639)tmp1=-119.1;
        else if(FT1<=0.30818)tmp1=-119;
    }
    else if(FT1<=0.32467){
        if(FT1==0.30818)tmp1=-119;
        else if(FT1<=0.30998)tmp1=-118.9;
        else if(FT1<=0.31179)tmp1=-118.8;
        else if(FT1<=0.31361)tmp1=-118.7;
        else if(FT1<=0.31543)tmp1=-118.6;
        else if(FT1<=0.31726)tmp1=-118.5;
        else if(FT1<=0.3191)tmp1=-118.4;
        else if(FT1<=0.32095)tmp1=-118.3;
        else if(FT1<=0.32281)tmp1=-118.2;
        else if(FT1<=0.32467)tmp1=-118.1;
        else if(FT1<=0.32655)tmp1=-118;
    }
    else if(FT1<=0.3438){
        if(FT1==0.32655)tmp1=-118;
        else if(FT1<=0.32843)tmp1=-117.9;
        else if(FT1<=0.33032)tmp1=-117.8;
        else if(FT1<=0.33222)tmp1=-117.7;
        else if(FT1<=0.33413)tmp1=-117.6;
        else if(FT1<=0.33605)tmp1=-117.5;
        else if(FT1<=0.33797)tmp1=-117.4;
        else if(FT1<=0.33991)tmp1=-117.3;
        else if(FT1<=0.34185)tmp1=-117.2;
        else if(FT1<=0.3438)tmp1=-117.1;
        else if(FT1<=0.34576)tmp1=-117;
    }
    else if(FT1<=0.36381){
        if(FT1==0.34576)tmp1=-117;
        else if(FT1<=0.34773)tmp1=-116.9;
        else if(FT1<=0.34971)tmp1=-116.8;
        else if(FT1<=0.3517)tmp1=-116.7;
        else if(FT1<=0.35369)tmp1=-116.6;
        else if(FT1<=0.3557)tmp1=-116.5;
        else if(FT1<=0.35771)tmp1=-116.4;
        else if(FT1<=0.35973)tmp1=-116.3;
        else if(FT1<=0.36176)tmp1=-116.2;
        else if(FT1<=0.36381)tmp1=-116.1;
        else if(FT1<=0.36585)tmp1=-116;
    }
    else if(FT1<=0.38471){
        if(FT1==0.36585)tmp1=-116;
        else if(FT1<=0.36791)tmp1=-115.9;
        else if(FT1<=0.36998)tmp1=-115.8;
        else if(FT1<=0.37206)tmp1=-115.7;
        else if(FT1<=0.37414)tmp1=-115.6;
        else if(FT1<=0.37624)tmp1=-115.5;
        else if(FT1<=0.37834)tmp1=-115.4;
        else if(FT1<=0.38046)tmp1=-115.3;
        else if(FT1<=0.38258)tmp1=-115.2;
        else if(FT1<=0.38471)tmp1=-115.1;
        else if(FT1<=0.38685)tmp1=-115;
    }
    else if(FT1<=0.40655){
        if(FT1==0.38685)tmp1=-115;
        else if(FT1<=0.389)tmp1=-114.9;
        else if(FT1<=0.39116)tmp1=-114.8;
        else if(FT1<=0.39333)tmp1=-114.7;
        else if(FT1<=0.39551)tmp1=-114.6;
        else if(FT1<=0.3977)tmp1=-114.5;
        else if(FT1<=0.3999)tmp1=-114.4;
        else if(FT1<=0.4021)tmp1=-114.3;
        else if(FT1<=0.40432)tmp1=-114.2;
        else if(FT1<=0.40655)tmp1=-114.1;
        else if(FT1<=0.40878)tmp1=-114;
    }
    else if(FT1<=0.42934){
        if(FT1==0.40878)tmp1=-114;
        else if(FT1<=0.41103)tmp1=-113.9;
        else if(FT1<=0.41328)tmp1=-113.8;
        else if(FT1<=0.41555)tmp1=-113.7;
        else if(FT1<=0.41782)tmp1=-113.6;
        else if(FT1<=0.4201)tmp1=-113.5;
        else if(FT1<=0.4224)tmp1=-113.4;
        else if(FT1<=0.4247)tmp1=-113.3;
        else if(FT1<=0.42702)tmp1=-113.2;
        else if(FT1<=0.42934)tmp1=-113.1;
        else if(FT1<=0.43167)tmp1=-113;
    }
    else if(FT1<=0.45312){
        if(FT1==0.43167)tmp1=-113;
        else if(FT1<=0.43401)tmp1=-112.9;
        else if(FT1<=0.43637)tmp1=-112.8;
        else if(FT1<=0.43873)tmp1=-112.7;
        else if(FT1<=0.4411)tmp1=-112.6;
        else if(FT1<=0.44349)tmp1=-112.5;
        else if(FT1<=0.44588)tmp1=-112.4;
        else if(FT1<=0.44828)tmp1=-112.3;
        else if(FT1<=0.45069)tmp1=-112.2;
        else if(FT1<=0.45312)tmp1=-112.1;
        else if(FT1<=0.45555)tmp1=-112;
    }
    else if(FT1<=0.47791){
        if(FT1==0.45555)tmp1=-112;
        else if(FT1<=0.45799)tmp1=-111.9;
        else if(FT1<=0.46045)tmp1=-111.8;
        else if(FT1<=0.46291)tmp1=-111.7;
        else if(FT1<=0.46538)tmp1=-111.6;
        else if(FT1<=0.46787)tmp1=-111.5;
        else if(FT1<=0.47036)tmp1=-111.4;
        else if(FT1<=0.47287)tmp1=-111.3;
        else if(FT1<=0.47538)tmp1=-111.2;
        else if(FT1<=0.47791)tmp1=-111.1;
        else if(FT1<=0.48045)tmp1=-111;
    }
    else if(FT1<=0.50375){
        if(FT1==0.48045)tmp1=-111;
        else if(FT1<=0.48299)tmp1=-110.9;
        else if(FT1<=0.48555)tmp1=-110.8;
        else if(FT1<=0.48812)tmp1=-110.7;
        else if(FT1<=0.4907)tmp1=-110.6;
        else if(FT1<=0.49328)tmp1=-110.5;
        else if(FT1<=0.49588)tmp1=-110.4;
        else if(FT1<=0.49849)tmp1=-110.3;
        else if(FT1<=0.50111)tmp1=-110.2;
        else if(FT1<=0.50375)tmp1=-110.1;
        else if(FT1<=0.50639)tmp1=-110;
    }
    else if(FT1<=0.53065){
        if(FT1==0.50639)tmp1=-110;
        else if(FT1<=0.50904)tmp1=-109.9;
        else if(FT1<=0.5117)tmp1=-109.8;
        else if(FT1<=0.51438)tmp1=-109.7;
        else if(FT1<=0.51706)tmp1=-109.6;
        else if(FT1<=0.51976)tmp1=-109.5;
        else if(FT1<=0.52247)tmp1=-109.4;
        else if(FT1<=0.52519)tmp1=-109.3;
        else if(FT1<=0.52791)tmp1=-109.2;
        else if(FT1<=0.53065)tmp1=-109.1;
        else if(FT1<=0.53341)tmp1=-109;
    }
    else if(FT1<=0.55867){
        if(FT1==0.53341)tmp1=-109;
        else if(FT1<=0.53617)tmp1=-108.9;
        else if(FT1<=0.53894)tmp1=-108.8;
        else if(FT1<=0.54173)tmp1=-108.7;
        else if(FT1<=0.54452)tmp1=-108.6;
        else if(FT1<=0.54733)tmp1=-108.5;
        else if(FT1<=0.55015)tmp1=-108.4;
        else if(FT1<=0.55297)tmp1=-108.3;
        else if(FT1<=0.55581)tmp1=-108.2;
        else if(FT1<=0.55867)tmp1=-108.1;
        else if(FT1<=0.56153)tmp1=-108;
    }
    else if(FT1<=0.58781){
        if(FT1==0.56153)tmp1=-108;
        else if(FT1<=0.5644)tmp1=-107.9;
        else if(FT1<=0.56729)tmp1=-107.8;
        else if(FT1<=0.57019)tmp1=-107.7;
        else if(FT1<=0.57309)tmp1=-107.6;
        else if(FT1<=0.57601)tmp1=-107.5;
        else if(FT1<=0.57895)tmp1=-107.4;
        else if(FT1<=0.58189)tmp1=-107.3;
        else if(FT1<=0.58484)tmp1=-107.2;
        else if(FT1<=0.58781)tmp1=-107.1;
        else if(FT1<=0.59079)tmp1=-107;
    }
    else if(FT1<=0.61811){
        if(FT1==0.59079)tmp1=-107;
        else if(FT1<=0.59378)tmp1=-106.9;
        else if(FT1<=0.59678)tmp1=-106.8;
        else if(FT1<=0.59979)tmp1=-106.7;
        else if(FT1<=0.60281)tmp1=-106.6;
        else if(FT1<=0.60585)tmp1=-106.5;
        else if(FT1<=0.6089)tmp1=-106.4;
        else if(FT1<=0.61196)tmp1=-106.3;
        else if(FT1<=0.61503)tmp1=-106.2;
        else if(FT1<=0.61811)tmp1=-106.1;
        else if(FT1<=0.62121)tmp1=-106;
    }
    else if(FT1<=0.64961){
        if(FT1==0.62121)tmp1=-106;
        else if(FT1<=0.62432)tmp1=-105.9;
        else if(FT1<=0.62744)tmp1=-105.8;
        else if(FT1<=0.63057)tmp1=-105.7;
        else if(FT1<=0.63371)tmp1=-105.6;
        else if(FT1<=0.63687)tmp1=-105.5;
        else if(FT1<=0.64004)tmp1=-105.4;
        else if(FT1<=0.64322)tmp1=-105.3;
        else if(FT1<=0.64641)tmp1=-105.2;
        else if(FT1<=0.64961)tmp1=-105.1;
        else if(FT1<=0.65283)tmp1=-105;
    }
    else if(FT1<=0.68233){
        if(FT1==0.65283)tmp1=-105;
        else if(FT1<=0.65606)tmp1=-104.9;
        else if(FT1<=0.6593)tmp1=-104.8;
        else if(FT1<=0.66255)tmp1=-104.7;
        else if(FT1<=0.66582)tmp1=-104.6;
        else if(FT1<=0.6691)tmp1=-104.5;
        else if(FT1<=0.67239)tmp1=-104.4;
        else if(FT1<=0.67569)tmp1=-104.3;
        else if(FT1<=0.679)tmp1=-104.2;
        else if(FT1<=0.68233)tmp1=-104.1;
        else if(FT1<=0.68567)tmp1=-104;
    }
    else if(FT1<=0.71631){
        if(FT1==0.68567)tmp1=-104;
        else if(FT1<=0.68903)tmp1=-103.9;
        else if(FT1<=0.69239)tmp1=-103.8;
        else if(FT1<=0.69577)tmp1=-103.7;
        else if(FT1<=0.69916)tmp1=-103.6;
        else if(FT1<=0.70256)tmp1=-103.5;
        else if(FT1<=0.70598)tmp1=-103.4;
        else if(FT1<=0.70941)tmp1=-103.3;
        else if(FT1<=0.71285)tmp1=-103.2;
        else if(FT1<=0.71631)tmp1=-103.1;
        else if(FT1<=0.71977)tmp1=-103;
    }
    else if(FT1<=0.75157){
        if(FT1==0.71977)tmp1=-103;
        else if(FT1<=0.72326)tmp1=-102.9;
        else if(FT1<=0.72675)tmp1=-102.8;
        else if(FT1<=0.73026)tmp1=-102.7;
        else if(FT1<=0.73377)tmp1=-102.6;
        else if(FT1<=0.73731)tmp1=-102.5;
        else if(FT1<=0.74085)tmp1=-102.4;
        else if(FT1<=0.74441)tmp1=-102.3;
        else if(FT1<=0.74798)tmp1=-102.2;
        else if(FT1<=0.75157)tmp1=-102.1;
        else if(FT1<=0.75516)tmp1=-102;
    }
    else if(FT1<=0.78814){
        if(FT1==0.75516)tmp1=-102;
        else if(FT1<=0.75878)tmp1=-101.9;
        else if(FT1<=0.7624)tmp1=-101.8;
        else if(FT1<=0.76604)tmp1=-101.7;
        else if(FT1<=0.76969)tmp1=-101.6;
        else if(FT1<=0.77335)tmp1=-101.5;
        else if(FT1<=0.77703)tmp1=-101.4;
        else if(FT1<=0.78072)tmp1=-101.3;
        else if(FT1<=0.78442)tmp1=-101.2;
        else if(FT1<=0.78814)tmp1=-101.1;
        else if(FT1<=0.79187)tmp1=-101;
    }
    else if(FT1<=0.82607){
        if(FT1==0.79187)tmp1=-101;
        else if(FT1<=0.79562)tmp1=-100.9;
        else if(FT1<=0.79938)tmp1=-100.8;
        else if(FT1<=0.80315)tmp1=-100.7;
        else if(FT1<=0.80693)tmp1=-100.6;
        else if(FT1<=0.81073)tmp1=-100.5;
        else if(FT1<=0.81455)tmp1=-100.4;
        else if(FT1<=0.81837)tmp1=-100.3;
        else if(FT1<=0.82221)tmp1=-100.2;
        else if(FT1<=0.82607)tmp1=-100.1;
        else if(FT1<=0.82993)tmp1=-100;
    }
    else if(FT1<=0.86537){
        if(FT1==0.82993)tmp1=-100;
        else if(FT1<=0.83382)tmp1=-99.9;
        else if(FT1<=0.83771)tmp1=-99.8;
        else if(FT1<=0.84162)tmp1=-99.7;
        else if(FT1<=0.84554)tmp1=-99.6;
        else if(FT1<=0.84948)tmp1=-99.5;
        else if(FT1<=0.85343)tmp1=-99.4;
        else if(FT1<=0.8574)tmp1=-99.3;
        else if(FT1<=0.86138)tmp1=-99.2;
        else if(FT1<=0.86537)tmp1=-99.1;
        else if(FT1<=0.86938)tmp1=-99;
    }
    else if(FT1<=0.90608){
        if(FT1==0.86938)tmp1=-99;
        else if(FT1<=0.8734)tmp1=-98.9;
        else if(FT1<=0.87743)tmp1=-98.8;
        else if(FT1<=0.88148)tmp1=-98.7;
        else if(FT1<=0.88555)tmp1=-98.6;
        else if(FT1<=0.88963)tmp1=-98.5;
        else if(FT1<=0.89372)tmp1=-98.4;
        else if(FT1<=0.89783)tmp1=-98.3;
        else if(FT1<=0.90195)tmp1=-98.2;
        else if(FT1<=0.90608)tmp1=-98.1;
        else if(FT1<=0.91023)tmp1=-98;
    }
    else if(FT1<=0.94824){
        if(FT1==0.91023)tmp1=-98;
        else if(FT1<=0.9144)tmp1=-97.9;
        else if(FT1<=0.91858)tmp1=-97.8;
        else if(FT1<=0.92277)tmp1=-97.7;
        else if(FT1<=0.92698)tmp1=-97.6;
        else if(FT1<=0.9312)tmp1=-97.5;
        else if(FT1<=0.93544)tmp1=-97.4;
        else if(FT1<=0.93969)tmp1=-97.3;
        else if(FT1<=0.94396)tmp1=-97.2;
        else if(FT1<=0.94824)tmp1=-97.1;
        else if(FT1<=0.95254)tmp1=-97;
    }
    else if(FT1<=0.99188){
        if(FT1==0.95254)tmp1=-97;
        else if(FT1<=0.95685)tmp1=-96.9;
        else if(FT1<=0.96118)tmp1=-96.8;
        else if(FT1<=0.96552)tmp1=-96.7;
        else if(FT1<=0.96987)tmp1=-96.6;
        else if(FT1<=0.97424)tmp1=-96.5;
        else if(FT1<=0.97863)tmp1=-96.4;
        else if(FT1<=0.98303)tmp1=-96.3;
        else if(FT1<=0.98745)tmp1=-96.2;
        else if(FT1<=0.99188)tmp1=-96.1;
        else if(FT1<=0.99632)tmp1=-96;
    }
    else if(FT1<=1.03702){
        if(FT1==0.99632)tmp1=-96;
        else if(FT1<=1.00078)tmp1=-95.9;
        else if(FT1<=1.00526)tmp1=-95.8;
        else if(FT1<=1.00975)tmp1=-95.7;
        else if(FT1<=1.01426)tmp1=-95.6;
        else if(FT1<=1.01878)tmp1=-95.5;
        else if(FT1<=1.02332)tmp1=-95.4;
        else if(FT1<=1.02787)tmp1=-95.3;
        else if(FT1<=1.03244)tmp1=-95.2;
        else if(FT1<=1.03702)tmp1=-95.1;
        else if(FT1<=1.04162)tmp1=-95;
    }
    else if(FT1<=1.0837){
        if(FT1==1.04162)tmp1=-95;
        else if(FT1<=1.04623)tmp1=-94.9;
        else if(FT1<=1.05086)tmp1=-94.8;
        else if(FT1<=1.05551)tmp1=-94.7;
        else if(FT1<=1.06017)tmp1=-94.6;
        else if(FT1<=1.06484)tmp1=-94.5;
        else if(FT1<=1.06954)tmp1=-94.4;
        else if(FT1<=1.07424)tmp1=-94.3;
        else if(FT1<=1.07897)tmp1=-94.2;
        else if(FT1<=1.0837)tmp1=-94.1;
        else if(FT1<=1.08846)tmp1=-94;
    }
    else if(FT1<=1.13196){
        if(FT1==1.08846)tmp1=-94;
        else if(FT1<=1.09323)tmp1=-93.9;
        else if(FT1<=1.09802)tmp1=-93.8;
        else if(FT1<=1.10282)tmp1=-93.7;
        else if(FT1<=1.10763)tmp1=-93.6;
        else if(FT1<=1.11247)tmp1=-93.5;
        else if(FT1<=1.11732)tmp1=-93.4;
        else if(FT1<=1.12218)tmp1=-93.3;
        else if(FT1<=1.12706)tmp1=-93.2;
        else if(FT1<=1.13196)tmp1=-93.1;
        else if(FT1<=1.13688)tmp1=-93;
    }
    else if(FT1<=1.18183){
        if(FT1==1.13688)tmp1=-93;
        else if(FT1<=1.14181)tmp1=-92.9;
        else if(FT1<=1.14675)tmp1=-92.8;
        else if(FT1<=1.15171)tmp1=-92.7;
        else if(FT1<=1.15669)tmp1=-92.6;
        else if(FT1<=1.16169)tmp1=-92.5;
        else if(FT1<=1.1667)tmp1=-92.4;
        else if(FT1<=1.17172)tmp1=-92.3;
        else if(FT1<=1.17677)tmp1=-92.2;
        else if(FT1<=1.18183)tmp1=-92.1;
        else if(FT1<=1.1869)tmp1=-92;
    }
    else if(FT1<=1.23333){
        if(FT1==1.1869)tmp1=-92;
        else if(FT1<=1.192)tmp1=-91.9;
        else if(FT1<=1.1971)tmp1=-91.8;
        else if(FT1<=1.20223)tmp1=-91.7;
        else if(FT1<=1.20737)tmp1=-91.6;
        else if(FT1<=1.21253)tmp1=-91.5;
        else if(FT1<=1.21771)tmp1=-91.4;
        else if(FT1<=1.2229)tmp1=-91.3;
        else if(FT1<=1.22811)tmp1=-91.2;
        else if(FT1<=1.23333)tmp1=-91.1;
        else if(FT1<=1.23857)tmp1=-91;
    }
    else if(FT1<=1.28651){
        if(FT1==1.23857)tmp1=-91;
        else if(FT1<=1.24383)tmp1=-90.9;
        else if(FT1<=1.24911)tmp1=-90.8;
        else if(FT1<=1.2544)tmp1=-90.7;
        else if(FT1<=1.25971)tmp1=-90.6;
        else if(FT1<=1.26503)tmp1=-90.5;
        else if(FT1<=1.27038)tmp1=-90.4;
        else if(FT1<=1.27574)tmp1=-90.3;
        else if(FT1<=1.28111)tmp1=-90.2;
        else if(FT1<=1.28651)tmp1=-90.1;
        else if(FT1<=1.29192)tmp1=-90;
    }
    else if(FT1<=1.34139){
        if(FT1==1.29192)tmp1=-90;
        else if(FT1<=1.29735)tmp1=-89.9;
        else if(FT1<=1.30279)tmp1=-89.8;
        else if(FT1<=1.30825)tmp1=-89.7;
        else if(FT1<=1.31373)tmp1=-89.6;
        else if(FT1<=1.31923)tmp1=-89.5;
        else if(FT1<=1.32475)tmp1=-89.4;
        else if(FT1<=1.33028)tmp1=-89.3;
        else if(FT1<=1.33583)tmp1=-89.2;
        else if(FT1<=1.34139)tmp1=-89.1;
        else if(FT1<=1.34697)tmp1=-89;
    }
    else if(FT1<=1.39801){
        if(FT1==1.34697)tmp1=-89;
        else if(FT1<=1.35258)tmp1=-88.9;
        else if(FT1<=1.35819)tmp1=-88.8;
        else if(FT1<=1.36383)tmp1=-88.7;
        else if(FT1<=1.36948)tmp1=-88.6;
        else if(FT1<=1.37515)tmp1=-88.5;
        else if(FT1<=1.38084)tmp1=-88.4;
        else if(FT1<=1.38655)tmp1=-88.3;
        else if(FT1<=1.39227)tmp1=-88.2;
        else if(FT1<=1.39801)tmp1=-88.1;
        else if(FT1<=1.40377)tmp1=-88;
    }
    else if(FT1<=1.45641){
        if(FT1==1.40377)tmp1=-88;
        else if(FT1<=1.40955)tmp1=-87.9;
        else if(FT1<=1.41534)tmp1=-87.8;
        else if(FT1<=1.42116)tmp1=-87.7;
        else if(FT1<=1.42699)tmp1=-87.6;
        else if(FT1<=1.43284)tmp1=-87.5;
        else if(FT1<=1.4387)tmp1=-87.4;
        else if(FT1<=1.44459)tmp1=-87.3;
        else if(FT1<=1.45049)tmp1=-87.2;
        else if(FT1<=1.45641)tmp1=-87.1;
        else if(FT1<=1.46235)tmp1=-87;
    }
    else if(FT1<=1.51661){
        if(FT1==1.46235)tmp1=-87;
        else if(FT1<=1.4683)tmp1=-86.9;
        else if(FT1<=1.47428)tmp1=-86.8;
        else if(FT1<=1.48027)tmp1=-86.7;
        else if(FT1<=1.48628)tmp1=-86.6;
        else if(FT1<=1.49231)tmp1=-86.5;
        else if(FT1<=1.49836)tmp1=-86.4;
        else if(FT1<=1.50442)tmp1=-86.3;
        else if(FT1<=1.51051)tmp1=-86.2;
        else if(FT1<=1.51661)tmp1=-86.1;
        else if(FT1<=1.52273)tmp1=-86;
    }
    else if(FT1<=1.57865){
        if(FT1==1.52273)tmp1=-86;
        else if(FT1<=1.52887)tmp1=-85.9;
        else if(FT1<=1.53503)tmp1=-85.8;
        else if(FT1<=1.5412)tmp1=-85.7;
        else if(FT1<=1.5474)tmp1=-85.6;
        else if(FT1<=1.55361)tmp1=-85.5;
        else if(FT1<=1.55984)tmp1=-85.4;
        else if(FT1<=1.56609)tmp1=-85.3;
        else if(FT1<=1.57236)tmp1=-85.2;
        else if(FT1<=1.57865)tmp1=-85.1;
        else if(FT1<=1.58496)tmp1=-85;
    }
    else if(FT1<=1.64256){
        if(FT1==1.58496)tmp1=-85;
        else if(FT1<=1.59128)tmp1=-84.9;
        else if(FT1<=1.59763)tmp1=-84.8;
        else if(FT1<=1.60399)tmp1=-84.7;
        else if(FT1<=1.61037)tmp1=-84.6;
        else if(FT1<=1.61677)tmp1=-84.5;
        else if(FT1<=1.62319)tmp1=-84.4;
        else if(FT1<=1.62963)tmp1=-84.3;
        else if(FT1<=1.63609)tmp1=-84.2;
        else if(FT1<=1.64256)tmp1=-84.1;
        else if(FT1<=1.64906)tmp1=-84;
    }
    else if(FT1<=1.70838){
        if(FT1==1.64906)tmp1=-84;
        else if(FT1<=1.65557)tmp1=-83.9;
        else if(FT1<=1.66211)tmp1=-83.8;
        else if(FT1<=1.66866)tmp1=-83.7;
        else if(FT1<=1.67523)tmp1=-83.6;
        else if(FT1<=1.68183)tmp1=-83.5;
        else if(FT1<=1.68844)tmp1=-83.4;
        else if(FT1<=1.69507)tmp1=-83.3;
        else if(FT1<=1.70172)tmp1=-83.2;
        else if(FT1<=1.70838)tmp1=-83.1;
        else if(FT1<=1.71507)tmp1=-83;
    }
    else if(FT1<=1.77615){
        if(FT1==1.71507)tmp1=-83;
        else if(FT1<=1.72178)tmp1=-82.9;
        else if(FT1<=1.72851)tmp1=-82.8;
        else if(FT1<=1.73525)tmp1=-82.7;
        else if(FT1<=1.74202)tmp1=-82.6;
        else if(FT1<=1.74881)tmp1=-82.5;
        else if(FT1<=1.75561)tmp1=-82.4;
        else if(FT1<=1.76244)tmp1=-82.3;
        else if(FT1<=1.76928)tmp1=-82.2;
        else if(FT1<=1.77615)tmp1=-82.1;
        else if(FT1<=1.78303)tmp1=-82;
    }
    else if(FT1<=1.84588){
        if(FT1==1.78303)tmp1=-82;
        else if(FT1<=1.78993)tmp1=-81.9;
        else if(FT1<=1.79686)tmp1=-81.8;
        else if(FT1<=1.8038)tmp1=-81.7;
        else if(FT1<=1.81076)tmp1=-81.6;
        else if(FT1<=1.81775)tmp1=-81.5;
        else if(FT1<=1.82475)tmp1=-81.4;
        else if(FT1<=1.83177)tmp1=-81.3;
        else if(FT1<=1.83882)tmp1=-81.2;
        else if(FT1<=1.84588)tmp1=-81.1;
        else if(FT1<=1.85296)tmp1=-81;
    }
    else if(FT1<=1.91762){
        if(FT1==1.85296)tmp1=-81;
        else if(FT1<=1.86007)tmp1=-80.9;
        else if(FT1<=1.86719)tmp1=-80.8;
        else if(FT1<=1.87433)tmp1=-80.7;
        else if(FT1<=1.8815)tmp1=-80.6;
        else if(FT1<=1.88868)tmp1=-80.5;
        else if(FT1<=1.89589)tmp1=-80.4;
        else if(FT1<=1.90311)tmp1=-80.3;
        else if(FT1<=1.91036)tmp1=-80.2;
        else if(FT1<=1.91762)tmp1=-80.1;
        else if(FT1<=1.92491)tmp1=-80;
    }
    else if(FT1<=1.99141){
        if(FT1==1.92491)tmp1=-80;
        else if(FT1<=1.93221)tmp1=-79.9;
        else if(FT1<=1.93954)tmp1=-79.8;
        else if(FT1<=1.94689)tmp1=-79.7;
        else if(FT1<=1.95426)tmp1=-79.6;
        else if(FT1<=1.96165)tmp1=-79.5;
        else if(FT1<=1.96905)tmp1=-79.4;
        else if(FT1<=1.97648)tmp1=-79.3;
        else if(FT1<=1.98394)tmp1=-79.2;
        else if(FT1<=1.99141)tmp1=-79.1;
        else if(FT1<=1.9989)tmp1=-79;
    }
    else if(FT1<=2.06727){
        if(FT1==1.9989)tmp1=-79;
        else if(FT1<=2.00641)tmp1=-78.9;
        else if(FT1<=2.01394)tmp1=-78.8;
        else if(FT1<=2.0215)tmp1=-78.7;
        else if(FT1<=2.02907)tmp1=-78.6;
        else if(FT1<=2.03667)tmp1=-78.5;
        else if(FT1<=2.04429)tmp1=-78.4;
        else if(FT1<=2.05193)tmp1=-78.3;
        else if(FT1<=2.05958)tmp1=-78.2;
        else if(FT1<=2.06727)tmp1=-78.1;
        else if(FT1<=2.07497)tmp1=-78;
    }
    else if(FT1<=2.14523){
        if(FT1==2.07497)tmp1=-78;
        else if(FT1<=2.08269)tmp1=-77.9;
        else if(FT1<=2.09043)tmp1=-77.8;
        else if(FT1<=2.0982)tmp1=-77.7;
        else if(FT1<=2.10598)tmp1=-77.6;
        else if(FT1<=2.11379)tmp1=-77.5;
        else if(FT1<=2.12162)tmp1=-77.4;
        else if(FT1<=2.12947)tmp1=-77.3;
        else if(FT1<=2.13734)tmp1=-77.2;
        else if(FT1<=2.14523)tmp1=-77.1;
        else if(FT1<=2.15315)tmp1=-77;
    }
    else if(FT1<=2.22534){
        if(FT1==2.15315)tmp1=-77;
        else if(FT1<=2.16108)tmp1=-76.9;
        else if(FT1<=2.16904)tmp1=-76.8;
        else if(FT1<=2.17702)tmp1=-76.7;
        else if(FT1<=2.18502)tmp1=-76.6;
        else if(FT1<=2.19304)tmp1=-76.5;
        else if(FT1<=2.20108)tmp1=-76.4;
        else if(FT1<=2.20915)tmp1=-76.3;
        else if(FT1<=2.21723)tmp1=-76.2;
        else if(FT1<=2.22534)tmp1=-76.1;
        else if(FT1<=2.23347)tmp1=-76;
    }
    else if(FT1<=2.30763){
        if(FT1==2.23347)tmp1=-76;
        else if(FT1<=2.24162)tmp1=-75.9;
        else if(FT1<=2.2498)tmp1=-75.8;
        else if(FT1<=2.25799)tmp1=-75.7;
        else if(FT1<=2.26621)tmp1=-75.6;
        else if(FT1<=2.27445)tmp1=-75.5;
        else if(FT1<=2.28271)tmp1=-75.4;
        else if(FT1<=2.29099)tmp1=-75.3;
        else if(FT1<=2.2993)tmp1=-75.2;
        else if(FT1<=2.30763)tmp1=-75.1;
        else if(FT1<=2.31598)tmp1=-75;
    }
    else if(FT1<=2.39212){
        if(FT1==2.31598)tmp1=-75;
        else if(FT1<=2.32435)tmp1=-74.9;
        else if(FT1<=2.33274)tmp1=-74.8;
        else if(FT1<=2.34116)tmp1=-74.7;
        else if(FT1<=2.3496)tmp1=-74.6;
        else if(FT1<=2.35806)tmp1=-74.5;
        else if(FT1<=2.36654)tmp1=-74.4;
        else if(FT1<=2.37504)tmp1=-74.3;
        else if(FT1<=2.38357)tmp1=-74.2;
        else if(FT1<=2.39212)tmp1=-74.1;
        else if(FT1<=2.40069)tmp1=-74;
    }
    else if(FT1<=2.47886){
        if(FT1==2.40069)tmp1=-74;
        else if(FT1<=2.40929)tmp1=-73.9;
        else if(FT1<=2.41791)tmp1=-73.8;
        else if(FT1<=2.42654)tmp1=-73.7;
        else if(FT1<=2.43521)tmp1=-73.6;
        else if(FT1<=2.44389)tmp1=-73.5;
        else if(FT1<=2.4526)tmp1=-73.4;
        else if(FT1<=2.46133)tmp1=-73.3;
        else if(FT1<=2.47008)tmp1=-73.2;
        else if(FT1<=2.47886)tmp1=-73.1;
        else if(FT1<=2.48766)tmp1=-73;
    }
    else if(FT1<=2.56787){
        if(FT1==2.48766)tmp1=-73;
        else if(FT1<=2.49648)tmp1=-72.9;
        else if(FT1<=2.50532)tmp1=-72.8;
        else if(FT1<=2.51419)tmp1=-72.7;
        else if(FT1<=2.52308)tmp1=-72.6;
        else if(FT1<=2.53199)tmp1=-72.5;
        else if(FT1<=2.54093)tmp1=-72.4;
        else if(FT1<=2.54988)tmp1=-72.3;
        else if(FT1<=2.55887)tmp1=-72.2;
        else if(FT1<=2.56787)tmp1=-72.1;
        else if(FT1<=2.5769)tmp1=-72;
    }
    else if(FT1<=2.65919){
        if(FT1==2.5769)tmp1=-72;
        else if(FT1<=2.58595)tmp1=-71.9;
        else if(FT1<=2.59502)tmp1=-71.8;
        else if(FT1<=2.60412)tmp1=-71.7;
        else if(FT1<=2.61324)tmp1=-71.6;
        else if(FT1<=2.62238)tmp1=-71.5;
        else if(FT1<=2.63155)tmp1=-71.4;
        else if(FT1<=2.64074)tmp1=-71.3;
        else if(FT1<=2.64996)tmp1=-71.2;
        else if(FT1<=2.65919)tmp1=-71.1;
        else if(FT1<=2.66845)tmp1=-71;
    }
    else if(FT1<=2.75286){
        if(FT1==2.66845)tmp1=-71;
        else if(FT1<=2.67774)tmp1=-70.9;
        else if(FT1<=2.68704)tmp1=-70.8;
        else if(FT1<=2.69638)tmp1=-70.7;
        else if(FT1<=2.70573)tmp1=-70.6;
        else if(FT1<=2.71511)tmp1=-70.5;
        else if(FT1<=2.72451)tmp1=-70.4;
        else if(FT1<=2.73394)tmp1=-70.3;
        else if(FT1<=2.74338)tmp1=-70.2;
        else if(FT1<=2.75286)tmp1=-70.1;
        else if(FT1<=2.76235)tmp1=-70;
    }
    else if(FT1<=2.8489){
        if(FT1==2.76235)tmp1=-70;
        else if(FT1<=2.77187)tmp1=-69.9;
        else if(FT1<=2.78142)tmp1=-69.8;
        else if(FT1<=2.79099)tmp1=-69.7;
        else if(FT1<=2.80058)tmp1=-69.6;
        else if(FT1<=2.81019)tmp1=-69.5;
        else if(FT1<=2.81983)tmp1=-69.4;
        else if(FT1<=2.8295)tmp1=-69.3;
        else if(FT1<=2.83919)tmp1=-69.2;
        else if(FT1<=2.8489)tmp1=-69.1;
        else if(FT1<=2.85863)tmp1=-69;
    }
    else if(FT1<=2.94735){
        if(FT1==2.85863)tmp1=-69;
        else if(FT1<=2.86839)tmp1=-68.9;
        else if(FT1<=2.87818)tmp1=-68.8;
        else if(FT1<=2.88799)tmp1=-68.7;
        else if(FT1<=2.89782)tmp1=-68.6;
        else if(FT1<=2.90768)tmp1=-68.5;
        else if(FT1<=2.91756)tmp1=-68.4;
        else if(FT1<=2.92746)tmp1=-68.3;
        else if(FT1<=2.93739)tmp1=-68.2;
        else if(FT1<=2.94735)tmp1=-68.1;
        else if(FT1<=2.95733)tmp1=-68;
    }
    else if(FT1<=3.04824){
        if(FT1==2.95733)tmp1=-68;
        else if(FT1<=2.96733)tmp1=-67.9;
        else if(FT1<=2.97736)tmp1=-67.8;
        else if(FT1<=2.98741)tmp1=-67.7;
        else if(FT1<=2.99748)tmp1=-67.6;
        else if(FT1<=3.00759)tmp1=-67.5;
        else if(FT1<=3.01771)tmp1=-67.4;
        else if(FT1<=3.02786)tmp1=-67.3;
        else if(FT1<=3.03804)tmp1=-67.2;
        else if(FT1<=3.04824)tmp1=-67.1;
        else if(FT1<=3.05846)tmp1=-67;
    }
    else if(FT1<=3.1516){
        if(FT1==3.05846)tmp1=-67;
        else if(FT1<=3.06871)tmp1=-66.9;
        else if(FT1<=3.07899)tmp1=-66.8;
        else if(FT1<=3.08928)tmp1=-66.7;
        else if(FT1<=3.09961)tmp1=-66.6;
        else if(FT1<=3.10996)tmp1=-66.5;
        else if(FT1<=3.12033)tmp1=-66.4;
        else if(FT1<=3.13073)tmp1=-66.3;
        else if(FT1<=3.14115)tmp1=-66.2;
        else if(FT1<=3.1516)tmp1=-66.1;
        else if(FT1<=3.16208)tmp1=-66;
    }
    else if(FT1<=3.25747){
        if(FT1==3.16208)tmp1=-66;
        else if(FT1<=3.17257)tmp1=-65.9;
        else if(FT1<=3.1831)tmp1=-65.8;
        else if(FT1<=3.19365)tmp1=-65.7;
        else if(FT1<=3.20422)tmp1=-65.6;
        else if(FT1<=3.21482)tmp1=-65.5;
        else if(FT1<=3.22545)tmp1=-65.4;
        else if(FT1<=3.2361)tmp1=-65.3;
        else if(FT1<=3.24677)tmp1=-65.2;
        else if(FT1<=3.25747)tmp1=-65.1;
        else if(FT1<=3.2682)tmp1=-65;
    }
    else if(FT1<=3.36589){
        if(FT1==3.2682)tmp1=-65;
        else if(FT1<=3.27895)tmp1=-64.9;
        else if(FT1<=3.28973)tmp1=-64.8;
        else if(FT1<=3.30053)tmp1=-64.7;
        else if(FT1<=3.31136)tmp1=-64.6;
        else if(FT1<=3.32221)tmp1=-64.5;
        else if(FT1<=3.33309)tmp1=-64.4;
        else if(FT1<=3.344)tmp1=-64.3;
        else if(FT1<=3.35493)tmp1=-64.2;
        else if(FT1<=3.36589)tmp1=-64.1;
        else if(FT1<=3.37687)tmp1=-64;
    }
    else if(FT1<=3.47687){
        if(FT1==3.37687)tmp1=-64;
        else if(FT1<=3.38788)tmp1=-63.9;
        else if(FT1<=3.39891)tmp1=-63.8;
        else if(FT1<=3.40997)tmp1=-63.7;
        else if(FT1<=3.42105)tmp1=-63.6;
        else if(FT1<=3.43217)tmp1=-63.5;
        else if(FT1<=3.4433)tmp1=-63.4;
        else if(FT1<=3.45447)tmp1=-63.3;
        else if(FT1<=3.46566)tmp1=-63.2;
        else if(FT1<=3.47687)tmp1=-63.1;
    }
    else if(FT1<=3.59046){
        if(FT1==3.48811)tmp1=-63;
        else if(FT1<=3.49938)tmp1=-62.9;
        else if(FT1<=3.51067)tmp1=-62.8;
        else if(FT1<=3.52199)tmp1=-62.7;
        else if(FT1<=3.53334)tmp1=-62.6;
        else if(FT1<=3.54471)tmp1=-62.5;
        else if(FT1<=3.55611)tmp1=-62.4;
        else if(FT1<=3.56753)tmp1=-62.3;
        else if(FT1<=3.57898)tmp1=-62.2;
        else if(FT1<=3.59046)tmp1=-62.1;
    }
    else if(FT1<=3.70668){
        if(FT1==3.60196)tmp1=-62;
        else if(FT1<=3.61349)tmp1=-61.9;
        else if(FT1<=3.62505)tmp1=-61.8;
        else if(FT1<=3.63663)tmp1=-61.7;
        else if(FT1<=3.64824)tmp1=-61.6;
        else if(FT1<=3.65988)tmp1=-61.5;
        else if(FT1<=3.67154)tmp1=-61.4;
        else if(FT1<=3.68323)tmp1=-61.3;
        else if(FT1<=3.69494)tmp1=-61.2;
        else if(FT1<=3.70668)tmp1=-61.1;
    }
    else if(FT1<=3.82558){
        if(FT1==3.71845)tmp1=-61;
        else if(FT1<=3.73025)tmp1=-60.9;
        else if(FT1<=3.74207)tmp1=-60.8;
        else if(FT1<=3.75392)tmp1=-60.7;
        else if(FT1<=3.76579)tmp1=-60.6;
        else if(FT1<=3.7777)tmp1=-60.5;
        else if(FT1<=3.78963)tmp1=-60.4;
        else if(FT1<=3.80158)tmp1=-60.3;
        else if(FT1<=3.81357)tmp1=-60.2;
        else if(FT1<=3.82558)tmp1=-60.1;
    }
    else if(FT1<=3.94717){
        if(FT1==3.83761)tmp1=-60;
        else if(FT1<=3.84968)tmp1=-59.9;
        else if(FT1<=3.86177)tmp1=-59.8;
        else if(FT1<=3.87389)tmp1=-59.7;
        else if(FT1<=3.88603)tmp1=-59.6;
        else if(FT1<=3.89821)tmp1=-59.5;
        else if(FT1<=3.91041)tmp1=-59.4;
        else if(FT1<=3.92264)tmp1=-59.3;
        else if(FT1<=3.93489)tmp1=-59.2;
        else if(FT1<=3.94717)tmp1=-59.1;
    }
    else if(FT1<=4.0715){
        if(FT1==3.95948)tmp1=-59;
        else if(FT1<=3.97182)tmp1=-58.9;
        else if(FT1<=3.98418)tmp1=-58.8;
        else if(FT1<=3.99657)tmp1=-58.7;
        else if(FT1<=4.00899)tmp1=-58.6;
        else if(FT1<=4.02144)tmp1=-58.5;
        else if(FT1<=4.03391)tmp1=-58.4;
        else if(FT1<=4.04641)tmp1=-58.3;
        else if(FT1<=4.05894)tmp1=-58.2;
        else if(FT1<=4.0715)tmp1=-58.1;
    }
    else if(FT1<=4.19859){
        if(FT1==4.08408)tmp1=-58;
        else if(FT1<=4.09669)tmp1=-57.9;
        else if(FT1<=4.10933)tmp1=-57.8;
        else if(FT1<=4.122)tmp1=-57.7;
        else if(FT1<=4.1347)tmp1=-57.6;
        else if(FT1<=4.14742)tmp1=-57.5;
        else if(FT1<=4.16017)tmp1=-57.4;
        else if(FT1<=4.17295)tmp1=-57.3;
        else if(FT1<=4.18575)tmp1=-57.2;
        else if(FT1<=4.19859)tmp1=-57.1;
    }
    else if(FT1<=4.32847){
        if(FT1==4.21145)tmp1=-57;
        else if(FT1<=4.22434)tmp1=-56.9;
        else if(FT1<=4.23726)tmp1=-56.8;
        else if(FT1<=4.2502)tmp1=-56.7;
        else if(FT1<=4.26318)tmp1=-56.6;
        else if(FT1<=4.27618)tmp1=-56.5;
        else if(FT1<=4.28921)tmp1=-56.4;
        else if(FT1<=4.30227)tmp1=-56.3;
        else if(FT1<=4.31536)tmp1=-56.2;
        else if(FT1<=4.32847)tmp1=-56.1;
    }
    else if(FT1<=4.46118){
        if(FT1==4.34162)tmp1=-56;
        else if(FT1<=4.35479)tmp1=-55.9;
        else if(FT1<=4.36799)tmp1=-55.8;
        else if(FT1<=4.38122)tmp1=-55.7;
        else if(FT1<=4.39447)tmp1=-55.6;
        else if(FT1<=4.40776)tmp1=-55.5;
        else if(FT1<=4.42107)tmp1=-55.4;
        else if(FT1<=4.43441)tmp1=-55.3;
        else if(FT1<=4.44778)tmp1=-55.2;
        else if(FT1<=4.46118)tmp1=-55.1;
    }
    else if(FT1<=4.59675){
        if(FT1==4.47461)tmp1=-55;
        else if(FT1<=4.48807)tmp1=-54.9;
        else if(FT1<=4.50155)tmp1=-54.8;
        else if(FT1<=4.51507)tmp1=-54.7;
        else if(FT1<=4.52861)tmp1=-54.6;
        else if(FT1<=4.54218)tmp1=-54.5;
        else if(FT1<=4.55578)tmp1=-54.4;
        else if(FT1<=4.56941)tmp1=-54.3;
        else if(FT1<=4.58306)tmp1=-54.2;
        else if(FT1<=4.59675)tmp1=-54.1;
    }
    else if(FT1<=4.73521){
        if(FT1==4.61047)tmp1=-54;
        else if(FT1<=4.62421)tmp1=-53.9;
        else if(FT1<=4.63798)tmp1=-53.8;
        else if(FT1<=4.65178)tmp1=-53.7;
        else if(FT1<=4.66562)tmp1=-53.6;
        else if(FT1<=4.67948)tmp1=-53.5;
        else if(FT1<=4.69336)tmp1=-53.4;
        else if(FT1<=4.70728)tmp1=-53.3;
        else if(FT1<=4.72123)tmp1=-53.2;
        else if(FT1<=4.73521)tmp1=-53.1;
    }
    else if(FT1<=4.87658){
        if(FT1==4.74921)tmp1=-53;
        else if(FT1<=4.76325)tmp1=-52.9;
        else if(FT1<=4.77731)tmp1=-52.8;
        else if(FT1<=4.7914)tmp1=-52.7;
        else if(FT1<=4.80553)tmp1=-52.6;
        else if(FT1<=4.81968)tmp1=-52.5;
        else if(FT1<=4.83386)tmp1=-52.4;
        else if(FT1<=4.84807)tmp1=-52.3;
        else if(FT1<=4.86231)tmp1=-52.2;
        else if(FT1<=4.87658)tmp1=-52.1;
    }
    else if(FT1<=5.0209){
        if(FT1==4.89088)tmp1=-52;
        else if(FT1<=4.90521)tmp1=-51.9;
        else if(FT1<=4.91957)tmp1=-51.8;
        else if(FT1<=4.93395)tmp1=-51.7;
        else if(FT1<=4.94837)tmp1=-51.6;
        else if(FT1<=4.96282)tmp1=-51.5;
        else if(FT1<=4.97729)tmp1=-51.4;
        else if(FT1<=4.9918)tmp1=-51.3;
        else if(FT1<=5.00634)tmp1=-51.2;
        else if(FT1<=5.0209)tmp1=-51.1;
    }
    else if(FT1<=5.1682){
        if(FT1==5.0355)tmp1=-51;
        else if(FT1<=5.05012)tmp1=-50.9;
        else if(FT1<=5.06478)tmp1=-50.8;
        else if(FT1<=5.07946)tmp1=-50.7;
        else if(FT1<=5.09418)tmp1=-50.6;
        else if(FT1<=5.10892)tmp1=-50.5;
        else if(FT1<=5.1237)tmp1=-50.4;
        else if(FT1<=5.1385)tmp1=-50.3;
        else if(FT1<=5.15334)tmp1=-50.2;
        else if(FT1<=5.1682)tmp1=-50.1;
    }
    else if(FT1<=5.31851){
        if(FT1==5.1831)tmp1=-50;
        else if(FT1<=5.19802)tmp1=-49.9;
        else if(FT1<=5.21298)tmp1=-49.8;
        else if(FT1<=5.22796)tmp1=-49.7;
        else if(FT1<=5.24298)tmp1=-49.6;
        else if(FT1<=5.25802)tmp1=-49.5;
        else if(FT1<=5.2731)tmp1=-49.4;
        else if(FT1<=5.28821)tmp1=-49.3;
        else if(FT1<=5.30334)tmp1=-49.2;
        else if(FT1<=5.31851)tmp1=-49.1;
    }
    else if(FT1<=5.47186){
        if(FT1==5.33371)tmp1=-49;
        else if(FT1<=5.34894)tmp1=-48.9;
        else if(FT1<=5.3642)tmp1=-48.8;
        else if(FT1<=5.37948)tmp1=-48.7;
        else if(FT1<=5.3948)tmp1=-48.6;
        else if(FT1<=5.41015)tmp1=-48.5;
        else if(FT1<=5.42553)tmp1=-48.4;
        else if(FT1<=5.44094)tmp1=-48.3;
        else if(FT1<=5.45639)tmp1=-48.2;
        else if(FT1<=5.47186)tmp1=-48.1;
    }
    else if(FT1<=5.62827){
        if(FT1==5.48736)tmp1=-48;
        else if(FT1<=5.5029)tmp1=-47.9;
        else if(FT1<=5.51846)tmp1=-47.8;
        else if(FT1<=5.53405)tmp1=-47.7;
        else if(FT1<=5.54968)tmp1=-47.6;
        else if(FT1<=5.56534)tmp1=-47.5;
        else if(FT1<=5.58103)tmp1=-47.4;
        else if(FT1<=5.59674)tmp1=-47.3;
        else if(FT1<=5.61249)tmp1=-47.2;
        else if(FT1<=5.62827)tmp1=-47.1;
    }
    else if(FT1<=5.78779){
        if(FT1==5.64408)tmp1=-47;
        else if(FT1<=5.65993)tmp1=-46.9;
        else if(FT1<=5.6758)tmp1=-46.8;
        else if(FT1<=5.69171)tmp1=-46.7;
        else if(FT1<=5.70764)tmp1=-46.6;
        else if(FT1<=5.72361)tmp1=-46.5;
        else if(FT1<=5.7396)tmp1=-46.4;
        else if(FT1<=5.75563)tmp1=-46.3;
        else if(FT1<=5.77169)tmp1=-46.2;
        else if(FT1<=5.78779)tmp1=-46.1;
    }
    else if(FT1<=5.95042){
        if(FT1==5.80391)tmp1=-46;
        else if(FT1<=5.82006)tmp1=-45.9;
        else if(FT1<=5.83625)tmp1=-45.8;
        else if(FT1<=5.85246)tmp1=-45.7;
        else if(FT1<=5.86871)tmp1=-45.6;
        else if(FT1<=5.88499)tmp1=-45.5;
        else if(FT1<=5.9013)tmp1=-45.4;
        else if(FT1<=5.91764)tmp1=-45.3;
        else if(FT1<=5.93402)tmp1=-45.2;
        else if(FT1<=5.95042)tmp1=-45.1;
    }
    else if(FT1<=6.11621){
        if(FT1==5.96686)tmp1=-45;
        else if(FT1<=5.98333)tmp1=-44.9;
        else if(FT1<=5.99983)tmp1=-44.8;
        else if(FT1<=6.01636)tmp1=-44.7;
        else if(FT1<=6.03292)tmp1=-44.6;
        else if(FT1<=6.04952)tmp1=-44.5;
        else if(FT1<=6.06614)tmp1=-44.4;
        else if(FT1<=6.0828)tmp1=-44.3;
        else if(FT1<=6.09949)tmp1=-44.2;
        else if(FT1<=6.11621)tmp1=-44.1;
    }
    else if(FT1<=6.28519){
        if(FT1==6.13297)tmp1=-44;
        else if(FT1<=6.14975)tmp1=-43.9;
        else if(FT1<=6.16657)tmp1=-43.8;
        else if(FT1<=6.18342)tmp1=-43.7;
        else if(FT1<=6.2003)tmp1=-43.6;
        else if(FT1<=6.21722)tmp1=-43.5;
        else if(FT1<=6.23416)tmp1=-43.4;
        else if(FT1<=6.25114)tmp1=-43.3;
        else if(FT1<=6.26815)tmp1=-43.2;
        else if(FT1<=6.28519)tmp1=-43.1;
    }
    else if(FT1<=6.45738){
        if(FT1==6.30226)tmp1=-43;
        else if(FT1<=6.31937)tmp1=-42.9;
        else if(FT1<=6.33651)tmp1=-42.8;
        else if(FT1<=6.35368)tmp1=-42.7;
        else if(FT1<=6.37088)tmp1=-42.6;
        else if(FT1<=6.38811)tmp1=-42.5;
        else if(FT1<=6.40538)tmp1=-42.4;
        else if(FT1<=6.42268)tmp1=-42.3;
        else if(FT1<=6.44001)tmp1=-42.2;
        else if(FT1<=6.45738)tmp1=-42.1;
    }
    else if(FT1<=6.6328){
        if(FT1==6.47477)tmp1=-42;
        else if(FT1<=6.4922)tmp1=-41.9;
        else if(FT1<=6.50966)tmp1=-41.8;
        else if(FT1<=6.52716)tmp1=-41.7;
        else if(FT1<=6.54468)tmp1=-41.6;
        else if(FT1<=6.56224)tmp1=-41.5;
        else if(FT1<=6.57983)tmp1=-41.4;
        else if(FT1<=6.59746)tmp1=-41.3;
        else if(FT1<=6.61511)tmp1=-41.2;
        else if(FT1<=6.6328)tmp1=-41.1;
    }
    else if(FT1<=6.81149){
        if(FT1==6.65052)tmp1=-41;
        else if(FT1<=6.66828)tmp1=-40.9;
        else if(FT1<=6.68606)tmp1=-40.8;
        else if(FT1<=6.70388)tmp1=-40.7;
        else if(FT1<=6.72174)tmp1=-40.6;
        else if(FT1<=6.73962)tmp1=-40.5;
        else if(FT1<=6.75754)tmp1=-40.4;
        else if(FT1<=6.77549)tmp1=-40.3;
        else if(FT1<=6.79348)tmp1=-40.2;
        else if(FT1<=6.81149)tmp1=-40.1;
    }
    else if(FT1<=6.99348){
        if(FT1==6.82954)tmp1=-40;
        else if(FT1<=6.84763)tmp1=-39.9;
        else if(FT1<=6.86574)tmp1=-39.8;
        else if(FT1<=6.88389)tmp1=-39.7;
        else if(FT1<=6.90207)tmp1=-39.6;
        else if(FT1<=6.92029)tmp1=-39.5;
        else if(FT1<=6.93854)tmp1=-39.4;
        else if(FT1<=6.95682)tmp1=-39.3;
        else if(FT1<=6.97513)tmp1=-39.2;
        else if(FT1<=6.99348)tmp1=-39.1;
    }
    else if(FT1<=7.17879){
        if(FT1==7.01186)tmp1=-39;
        else if(FT1<=7.03028)tmp1=-38.9;
        else if(FT1<=7.04872)tmp1=-38.8;
        else if(FT1<=7.0672)tmp1=-38.7;
        else if(FT1<=7.08572)tmp1=-38.6;
        else if(FT1<=7.10427)tmp1=-38.5;
        else if(FT1<=7.12285)tmp1=-38.4;
        else if(FT1<=7.14146)tmp1=-38.3;
        else if(FT1<=7.16011)tmp1=-38.2;
        else if(FT1<=7.17879)tmp1=-38.1;
    }
    else if(FT1<=7.36745){
        if(FT1==7.1975)tmp1=-38;
        else if(FT1<=7.21625)tmp1=-37.9;
        else if(FT1<=7.23503)tmp1=-37.8;
        else if(FT1<=7.25385)tmp1=-37.7;
        else if(FT1<=7.2727)tmp1=-37.6;
        else if(FT1<=7.29158)tmp1=-37.5;
        else if(FT1<=7.3105)tmp1=-37.4;
        else if(FT1<=7.32944)tmp1=-37.3;
        else if(FT1<=7.34843)tmp1=-37.2;
        else if(FT1<=7.36745)tmp1=-37.1;
    }
    else if(FT1<=7.55948){
        if(FT1==7.3865)tmp1=-37;
        else if(FT1<=7.40558)tmp1=-36.9;
        else if(FT1<=7.4247)tmp1=-36.8;
        else if(FT1<=7.44385)tmp1=-36.7;
        else if(FT1<=7.46304)tmp1=-36.6;
        else if(FT1<=7.48226)tmp1=-36.5;
        else if(FT1<=7.50151)tmp1=-36.4;
        else if(FT1<=7.5208)tmp1=-36.3;
        else if(FT1<=7.54012)tmp1=-36.2;
        else if(FT1<=7.55948)tmp1=-36.1;
    }
    else if(FT1<=7.75491){
        if(FT1==7.57887)tmp1=-36;
        else if(FT1<=7.59829)tmp1=-35.9;
        else if(FT1<=7.61775)tmp1=-35.8;
        else if(FT1<=7.63724)tmp1=-35.7;
        else if(FT1<=7.65677)tmp1=-35.6;
        else if(FT1<=7.67633)tmp1=-35.5;
        else if(FT1<=7.69592)tmp1=-35.4;
        else if(FT1<=7.71555)tmp1=-35.3;
        else if(FT1<=7.73521)tmp1=-35.2;
        else if(FT1<=7.75491)tmp1=-35.1;
    }
    else if(FT1<=7.95377){
        if(FT1==7.77464)tmp1=-35;
        else if(FT1<=7.79441)tmp1=-34.9;
        else if(FT1<=7.81421)tmp1=-34.8;
        else if(FT1<=7.83404)tmp1=-34.7;
        else if(FT1<=7.85391)tmp1=-34.6;
        else if(FT1<=7.87381)tmp1=-34.5;
        else if(FT1<=7.89375)tmp1=-34.4;
        else if(FT1<=7.91372)tmp1=-34.3;
        else if(FT1<=7.93373)tmp1=-34.2;
        else if(FT1<=7.95377)tmp1=-34.1;
    }
    else if(FT1<=8.15609){
        if(FT1==7.97385)tmp1=-34;
        else if(FT1<=7.99396)tmp1=-33.9;
        else if(FT1<=8.0141)tmp1=-33.8;
        else if(FT1<=8.03428)tmp1=-33.7;
        else if(FT1<=8.0545)tmp1=-33.6;
        else if(FT1<=8.07474)tmp1=-33.5;
        else if(FT1<=8.09503)tmp1=-33.4;
        else if(FT1<=8.11535)tmp1=-33.3;
        else if(FT1<=8.1357)tmp1=-33.2;
        else if(FT1<=8.15609)tmp1=-33.1;
    }
    else if(FT1<=8.36188){
        if(FT1==8.17651)tmp1=-33;
        else if(FT1<=8.19697)tmp1=-32.9;
        else if(FT1<=8.21746)tmp1=-32.8;
        else if(FT1<=8.23798)tmp1=-32.7;
        else if(FT1<=8.25855)tmp1=-32.6;
        else if(FT1<=8.27914)tmp1=-32.5;
        else if(FT1<=8.29978)tmp1=-32.4;
        else if(FT1<=8.32044)tmp1=-32.3;
        else if(FT1<=8.34114)tmp1=-32.2;
        else if(FT1<=8.36188)tmp1=-32.1;
    }
    else if(FT1<=8.57118){
        if(FT1==8.38265)tmp1=-32;
        else if(FT1<=8.40346)tmp1=-31.9;
        else if(FT1<=8.4243)tmp1=-31.8;
        else if(FT1<=8.44518)tmp1=-31.7;
        else if(FT1<=8.46609)tmp1=-31.6;
        else if(FT1<=8.48704)tmp1=-31.5;
        else if(FT1<=8.50802)tmp1=-31.4;
        else if(FT1<=8.52904)tmp1=-31.3;
        else if(FT1<=8.55009)tmp1=-31.2;
        else if(FT1<=8.57118)tmp1=-31.1;
    }
    else if(FT1<=8.78401){
        if(FT1==8.5923)tmp1=-31;
        else if(FT1<=8.61346)tmp1=-30.9;
        else if(FT1<=8.63465)tmp1=-30.8;
        else if(FT1<=8.65588)tmp1=-30.7;
        else if(FT1<=8.67715)tmp1=-30.6;
        else if(FT1<=8.69845)tmp1=-30.5;
        else if(FT1<=8.71979)tmp1=-30.4;
        else if(FT1<=8.74116)tmp1=-30.3;
        else if(FT1<=8.76256)tmp1=-30.2;
        else if(FT1<=8.78401)tmp1=-30.1;
    }
    else if(FT1<=9.00039){
        if(FT1==8.80548)tmp1=-30;
        else if(FT1<=8.827)tmp1=-29.9;
        else if(FT1<=8.84855)tmp1=-29.8;
        else if(FT1<=8.87013)tmp1=-29.7;
        else if(FT1<=8.89175)tmp1=-29.6;
        else if(FT1<=8.91341)tmp1=-29.5;
        else if(FT1<=8.9351)tmp1=-29.4;
        else if(FT1<=8.95682)tmp1=-29.3;
        else if(FT1<=8.97859)tmp1=-29.2;
        else if(FT1<=9.00039)tmp1=-29.1;
    }
    else if(FT1<=9.22035){
        if(FT1==9.02222)tmp1=-29;
        else if(FT1<=9.04409)tmp1=-28.9;
        else if(FT1<=9.066)tmp1=-28.8;
        else if(FT1<=9.08794)tmp1=-28.7;
        else if(FT1<=9.10992)tmp1=-28.6;
        else if(FT1<=9.13193)tmp1=-28.5;
        else if(FT1<=9.15398)tmp1=-28.4;
        else if(FT1<=9.17607)tmp1=-28.3;
        else if(FT1<=9.19819)tmp1=-28.2;
        else if(FT1<=9.22035)tmp1=-28.1;
    }
    else if(FT1<=9.44391){
        if(FT1==9.24254)tmp1=-28;
        else if(FT1<=9.26477)tmp1=-27.9;
        else if(FT1<=9.28704)tmp1=-27.8;
        else if(FT1<=9.30934)tmp1=-27.7;
        else if(FT1<=9.33168)tmp1=-27.6;
        else if(FT1<=9.35405)tmp1=-27.5;
        else if(FT1<=9.37646)tmp1=-27.4;
        else if(FT1<=9.39891)tmp1=-27.3;
        else if(FT1<=9.42139)tmp1=-27.2;
        else if(FT1<=9.44391)tmp1=-27.1;
    }
    else if(FT1<=9.6711){
        if(FT1==9.46646)tmp1=-27;
        else if(FT1<=9.48906)tmp1=-26.9;
        else if(FT1<=9.51168)tmp1=-26.8;
        else if(FT1<=9.53435)tmp1=-26.7;
        else if(FT1<=9.55705)tmp1=-26.6;
        else if(FT1<=9.57979)tmp1=-26.5;
        else if(FT1<=9.60256)tmp1=-26.4;
        else if(FT1<=9.62537)tmp1=-26.3;
        else if(FT1<=9.64822)tmp1=-26.2;
        else if(FT1<=9.6711)tmp1=-26.1;
    }
    else if(FT1<=9.90194){
        if(FT1==9.69402)tmp1=-26;
        else if(FT1<=9.71697)tmp1=-25.9;
        else if(FT1<=9.73997)tmp1=-25.8;
        else if(FT1<=9.76299)tmp1=-25.7;
        else if(FT1<=9.78606)tmp1=-25.6;
        else if(FT1<=9.80916)tmp1=-25.5;
        else if(FT1<=9.8323)tmp1=-25.4;
        else if(FT1<=9.85548)tmp1=-25.3;
        else if(FT1<=9.87869)tmp1=-25.2;
        else if(FT1<=9.90194)tmp1=-25.1;
    }
    else if(FT1<=10.1364){
        if(FT1==9.92522)tmp1=-25;
        else if(FT1<=9.94854)tmp1=-24.9;
        else if(FT1<=9.9719)tmp1=-24.8;
        else if(FT1<=9.9953)tmp1=-24.7;
        else if(FT1<=10.0187)tmp1=-24.6;
        else if(FT1<=10.0422)tmp1=-24.5;
        else if(FT1<=10.0657)tmp1=-24.4;
        else if(FT1<=10.0893)tmp1=-24.3;
        else if(FT1<=10.1128)tmp1=-24.2;
        else if(FT1<=10.1364)tmp1=-24.1;
    }
    else if(FT1<=10.3747){
        if(FT1==10.1601)tmp1=-24;
        else if(FT1<=10.1838)tmp1=-23.9;
        else if(FT1<=10.2075)tmp1=-23.8;
        else if(FT1<=10.2313)tmp1=-23.7;
        else if(FT1<=10.2551)tmp1=-23.6;
        else if(FT1<=10.2789)tmp1=-23.5;
        else if(FT1<=10.3028)tmp1=-23.4;
        else if(FT1<=10.3267)tmp1=-23.3;
        else if(FT1<=10.3507)tmp1=-23.2;
        else if(FT1<=10.3747)tmp1=-23.1;
    }
    else if(FT1<=10.6166){
        if(FT1==10.3987)tmp1=-23;
        else if(FT1<=10.4227)tmp1=-22.9;
        else if(FT1<=10.4468)tmp1=-22.8;
        else if(FT1<=10.471)tmp1=-22.7;
        else if(FT1<=10.4952)tmp1=-22.6;
        else if(FT1<=10.5194)tmp1=-22.5;
        else if(FT1<=10.5436)tmp1=-22.4;
        else if(FT1<=10.5679)tmp1=-22.3;
        else if(FT1<=10.5922)tmp1=-22.2;
        else if(FT1<=10.6166)tmp1=-22.1;
    }
    else if(FT1<=10.8623){
        if(FT1==10.641)tmp1=-22;
        else if(FT1<=10.6654)tmp1=-21.9;
        else if(FT1<=10.6899)tmp1=-21.8;
        else if(FT1<=10.7144)tmp1=-21.7;
        else if(FT1<=10.739)tmp1=-21.6;
        else if(FT1<=10.7635)tmp1=-21.5;
        else if(FT1<=10.7882)tmp1=-21.4;
        else if(FT1<=10.8128)tmp1=-21.3;
        else if(FT1<=10.8375)tmp1=-21.2;
        else if(FT1<=10.8623)tmp1=-21.1;
    }
    else if(FT1<=11.1117){
        if(FT1==10.887)tmp1=-21;
        else if(FT1<=10.9118)tmp1=-20.9;
        else if(FT1<=10.9367)tmp1=-20.8;
        else if(FT1<=10.9616)tmp1=-20.7;
        else if(FT1<=10.9865)tmp1=-20.6;
        else if(FT1<=11.0115)tmp1=-20.5;
        else if(FT1<=11.0365)tmp1=-20.4;
        else if(FT1<=11.0615)tmp1=-20.3;
        else if(FT1<=11.0866)tmp1=-20.2;
        else if(FT1<=11.1117)tmp1=-20.1;
    }
    else if(FT1<=11.3649){
        if(FT1==11.1368)tmp1=-20;
        else if(FT1<=11.162)tmp1=-19.9;
        else if(FT1<=11.1873)tmp1=-19.8;
        else if(FT1<=11.2125)tmp1=-19.7;
        else if(FT1<=11.2378)tmp1=-19.6;
        else if(FT1<=11.2632)tmp1=-19.5;
        else if(FT1<=11.2885)tmp1=-19.4;
        else if(FT1<=11.314)tmp1=-19.3;
        else if(FT1<=11.3394)tmp1=-19.2;
        else if(FT1<=11.3649)tmp1=-19.1;
    }
    else if(FT1<=11.6219){
        if(FT1==11.3904)tmp1=-19;
        else if(FT1<=11.416)tmp1=-18.9;
        else if(FT1<=11.4416)tmp1=-18.8;
        else if(FT1<=11.4673)tmp1=-18.7;
        else if(FT1<=11.4929)tmp1=-18.6;
        else if(FT1<=11.5187)tmp1=-18.5;
        else if(FT1<=11.5444)tmp1=-18.4;
        else if(FT1<=11.5702)tmp1=-18.3;
        else if(FT1<=11.5961)tmp1=-18.2;
        else if(FT1<=11.6219)tmp1=-18.1;
    }
    else if(FT1<=11.8828){
        if(FT1==11.6478)tmp1=-18;
        else if(FT1<=11.6738)tmp1=-17.9;
        else if(FT1<=11.6998)tmp1=-17.8;
        else if(FT1<=11.7258)tmp1=-17.7;
        else if(FT1<=11.7519)tmp1=-17.6;
        else if(FT1<=11.778)tmp1=-17.5;
        else if(FT1<=11.8041)tmp1=-17.4;
        else if(FT1<=11.8303)tmp1=-17.3;
        else if(FT1<=11.8565)tmp1=-17.2;
        else if(FT1<=11.8828)tmp1=-17.1;
    }
    else if(FT1<=12.1475){
        if(FT1==11.9091)tmp1=-17;
        else if(FT1<=11.9354)tmp1=-16.9;
        else if(FT1<=11.9618)tmp1=-16.8;
        else if(FT1<=11.9882)tmp1=-16.7;
        else if(FT1<=12.0146)tmp1=-16.6;
        else if(FT1<=12.0411)tmp1=-16.5;
        else if(FT1<=12.0677)tmp1=-16.4;
        else if(FT1<=12.0942)tmp1=-16.3;
        else if(FT1<=12.1208)tmp1=-16.2;
        else if(FT1<=12.1475)tmp1=-16.1;
    }
    else if(FT1<=12.416){
        if(FT1==12.1742)tmp1=-16;
        else if(FT1<=12.2009)tmp1=-15.9;
        else if(FT1<=12.2276)tmp1=-15.8;
        else if(FT1<=12.2544)tmp1=-15.7;
        else if(FT1<=12.2813)tmp1=-15.6;
        else if(FT1<=12.3082)tmp1=-15.5;
        else if(FT1<=12.3351)tmp1=-15.4;
        else if(FT1<=12.362)tmp1=-15.3;
        else if(FT1<=12.389)tmp1=-15.2;
        else if(FT1<=12.416)tmp1=-15.1;
        else if(FT1<=12.4431)tmp1=-15;
    }
    else if(FT1<=12.6885){
        if(FT1==12.4431)tmp1=-15;
        else if(FT1<=12.4702)tmp1=-14.9;
        else if(FT1<=12.4974)tmp1=-14.8;
        else if(FT1<=12.5246)tmp1=-14.7;
        else if(FT1<=12.5518)tmp1=-14.6;
        else if(FT1<=12.5791)tmp1=-14.5;
        else if(FT1<=12.6064)tmp1=-14.4;
        else if(FT1<=12.6337)tmp1=-14.3;
        else if(FT1<=12.6611)tmp1=-14.2;
        else if(FT1<=12.6885)tmp1=-14.1;
        else if(FT1<=12.716)tmp1=-14;
    }
    else if(FT1<=12.9649){
        if(FT1==12.716)tmp1=-14;
        else if(FT1<=12.7435)tmp1=-13.9;
        else if(FT1<=12.771)tmp1=-13.8;
        else if(FT1<=12.7986)tmp1=-13.7;
        else if(FT1<=12.8262)tmp1=-13.6;
        else if(FT1<=12.8539)tmp1=-13.5;
        else if(FT1<=12.8816)tmp1=-13.4;
        else if(FT1<=12.9093)tmp1=-13.3;
        else if(FT1<=12.9371)tmp1=-13.2;
        else if(FT1<=12.9649)tmp1=-13.1;
        else if(FT1<=12.9927)tmp1=-13;
    }
    else if(FT1<=13.2452){
        if(FT1==12.9927)tmp1=-13;
        else if(FT1<=13.0206)tmp1=-12.9;
        else if(FT1<=13.0485)tmp1=-12.8;
        else if(FT1<=13.0765)tmp1=-12.7;
        else if(FT1<=13.1045)tmp1=-12.6;
        else if(FT1<=13.1326)tmp1=-12.5;
        else if(FT1<=13.1607)tmp1=-12.4;
        else if(FT1<=13.1888)tmp1=-12.3;
        else if(FT1<=13.217)tmp1=-12.2;
        else if(FT1<=13.2452)tmp1=-12.1;
        else if(FT1<=13.2734)tmp1=-12;
    }
    else if(FT1<=13.5294){
        if(FT1==13.2734)tmp1=-12;
        else if(FT1<=13.3017)tmp1=-11.9;
        else if(FT1<=13.33)tmp1=-11.8;
        else if(FT1<=13.3584)tmp1=-11.7;
        else if(FT1<=13.3868)tmp1=-11.6;
        else if(FT1<=13.4152)tmp1=-11.5;
        else if(FT1<=13.4437)tmp1=-11.4;
        else if(FT1<=13.4722)tmp1=-11.3;
        else if(FT1<=13.5008)tmp1=-11.2;
        else if(FT1<=13.5294)tmp1=-11.1;
        else if(FT1<=13.5581)tmp1=-11;
    }
    else if(FT1<=13.8176){
        if(FT1==13.5581)tmp1=-11;
        else if(FT1<=13.5867)tmp1=-10.9;
        else if(FT1<=13.6155)tmp1=-10.8;
        else if(FT1<=13.6442)tmp1=-10.7;
        else if(FT1<=13.673)tmp1=-10.6;
        else if(FT1<=13.7019)tmp1=-10.5;
        else if(FT1<=13.7307)tmp1=-10.4;
        else if(FT1<=13.7597)tmp1=-10.3;
        else if(FT1<=13.7886)tmp1=-10.2;
        else if(FT1<=13.8176)tmp1=-10.1;
        else if(FT1<=13.8467)tmp1=-10;
    }
    else if(FT1<=14.1098){
        if(FT1==13.8467)tmp1=-10;
        else if(FT1<=13.8757)tmp1=-9.9;
        else if(FT1<=13.9049)tmp1=-9.8;
        else if(FT1<=13.934)tmp1=-9.7;
        else if(FT1<=13.9632)tmp1=-9.6;
        else if(FT1<=13.9925)tmp1=-9.5;
        else if(FT1<=14.0217)tmp1=-9.4;
        else if(FT1<=14.0511)tmp1=-9.3;
        else if(FT1<=14.0804)tmp1=-9.2;
        else if(FT1<=14.1098)tmp1=-9.1;
        else if(FT1<=14.1393)tmp1=-9;
    }
    else if(FT1<=14.406){
        if(FT1==14.1393)tmp1=-9;
        else if(FT1<=14.1687)tmp1=-8.9;
        else if(FT1<=14.1983)tmp1=-8.8;
        else if(FT1<=14.2278)tmp1=-8.7;
        else if(FT1<=14.2574)tmp1=-8.6;
        else if(FT1<=14.2871)tmp1=-8.5;
        else if(FT1<=14.3167)tmp1=-8.4;
        else if(FT1<=14.3465)tmp1=-8.3;
        else if(FT1<=14.3762)tmp1=-8.2;
        else if(FT1<=14.406)tmp1=-8.1;
        else if(FT1<=14.4359)tmp1=-8;
    }
    else if(FT1<=14.7063){
        if(FT1==14.4359)tmp1=-8;
        else if(FT1<=14.4657)tmp1=-7.9;
        else if(FT1<=14.4957)tmp1=-7.8;
        else if(FT1<=14.5256)tmp1=-7.7;
        else if(FT1<=14.5556)tmp1=-7.6;
        else if(FT1<=14.5857)tmp1=-7.5;
        else if(FT1<=14.6158)tmp1=-7.4;
        else if(FT1<=14.6459)tmp1=-7.3;
        else if(FT1<=14.676)tmp1=-7.2;
        else if(FT1<=14.7063)tmp1=-7.1;
        else if(FT1<=14.7365)tmp1=-7;
    }
    else if(FT1<=15.0105){
        if(FT1==14.7365)tmp1=-7;
        else if(FT1<=14.7668)tmp1=-6.9;
        else if(FT1<=14.7971)tmp1=-6.8;
        else if(FT1<=14.8275)tmp1=-6.7;
        else if(FT1<=14.8579)tmp1=-6.6;
        else if(FT1<=14.8883)tmp1=-6.5;
        else if(FT1<=14.9188)tmp1=-6.4;
        else if(FT1<=14.9493)tmp1=-6.3;
        else if(FT1<=14.9799)tmp1=-6.2;
        else if(FT1<=15.0105)tmp1=-6.1;
        else if(FT1<=15.0412)tmp1=-6;
    }
    else if(FT1<=15.3188){
        if(FT1==15.0412)tmp1=-6;
        else if(FT1<=15.0719)tmp1=-5.9;
        else if(FT1<=15.1026)tmp1=-5.8;
        else if(FT1<=15.1334)tmp1=-5.7;
        else if(FT1<=15.1642)tmp1=-5.6;
        else if(FT1<=15.195)tmp1=-5.5;
        else if(FT1<=15.2259)tmp1=-5.4;
        else if(FT1<=15.2568)tmp1=-5.3;
        else if(FT1<=15.2878)tmp1=-5.2;
        else if(FT1<=15.3188)tmp1=-5.1;
        else if(FT1<=15.3499)tmp1=-5;
    }
    else if(FT1<=15.6312){
        if(FT1==15.3499)tmp1=-5;
        else if(FT1<=15.381)tmp1=-4.9;
        else if(FT1<=15.4121)tmp1=-4.8;
        else if(FT1<=15.4433)tmp1=-4.7;
        else if(FT1<=15.4745)tmp1=-4.6;
        else if(FT1<=15.5058)tmp1=-4.5;
        else if(FT1<=15.5371)tmp1=-4.4;
        else if(FT1<=15.5684)tmp1=-4.3;
        else if(FT1<=15.5998)tmp1=-4.2;
        else if(FT1<=15.6312)tmp1=-4.1;
        else if(FT1<=15.6627)tmp1=-4;
    }
    else if(FT1<=15.9477){
        if(FT1==15.6627)tmp1=-4;
        else if(FT1<=15.6942)tmp1=-3.9;
        else if(FT1<=15.7258)tmp1=-3.8;
        else if(FT1<=15.7573)tmp1=-3.7;
        else if(FT1<=15.789)tmp1=-3.6;
        else if(FT1<=15.8206)tmp1=-3.5;
        else if(FT1<=15.8524)tmp1=-3.4;
        else if(FT1<=15.8841)tmp1=-3.3;
        else if(FT1<=15.9159)tmp1=-3.2;
        else if(FT1<=15.9477)tmp1=-3.1;
    }
    else if(FT1<=16.2683){
        if(FT1==15.9796)tmp1=-3;
        else if(FT1<=16.0115)tmp1=-2.9;
        else if(FT1<=16.0435)tmp1=-2.8;
        else if(FT1<=16.0755)tmp1=-2.7;
        else if(FT1<=16.1075)tmp1=-2.6;
        else if(FT1<=16.1396)tmp1=-2.5;
        else if(FT1<=16.1717)tmp1=-2.4;
        else if(FT1<=16.2039)tmp1=-2.3;
        else if(FT1<=16.2361)tmp1=-2.2;
        else if(FT1<=16.2683)tmp1=-2.1;
    }
    else if(FT1<=16.5931){
        if(FT1==16.3006)tmp1=-2;
        else if(FT1<=16.3329)tmp1=-1.9;
        else if(FT1<=16.3653)tmp1=-1.8;
        else if(FT1<=16.3977)tmp1=-1.7;
        else if(FT1<=16.4302)tmp1=-1.6;
        else if(FT1<=16.4627)tmp1=-1.5;
        else if(FT1<=16.4952)tmp1=-1.4;
        else if(FT1<=16.5278)tmp1=-1.3;
        else if(FT1<=16.5604)tmp1=-1.2;
        else if(FT1<=16.5931)tmp1=-1.1;
    }
    else if(FT1<=16.9219){
        if(FT1==16.6258)tmp1=-1;
        else if(FT1<=16.6585)tmp1=-0.9;
        else if(FT1<=16.6913)tmp1=-0.8;
        else if(FT1<=16.7241)tmp1=-0.7;
        else if(FT1<=16.757)tmp1=-0.6;
        else if(FT1<=16.7899)tmp1=-0.5;
        else if(FT1<=16.8228)tmp1=-0.4;
        else if(FT1<=16.8558)tmp1=-0.3;
        else if(FT1<=16.8888)tmp1=-0.2;
        else if(FT1<=16.9219)tmp1=-0.1;
        else if(FT1<=16.955)tmp1=-1.38778e-16;
    }
    else if(FT1<=17.2549){
        if(FT1==16.955)tmp1=0;
        else if(FT1<=16.9882)tmp1=0.1;
        else if(FT1<=17.0214)tmp1=0.2;
        else if(FT1<=17.0546)tmp1=0.3;
        else if(FT1<=17.0879)tmp1=0.4;
        else if(FT1<=17.1212)tmp1=0.5;
        else if(FT1<=17.1546)tmp1=0.6;
        else if(FT1<=17.188)tmp1=0.7;
        else if(FT1<=17.2215)tmp1=0.8;
        else if(FT1<=17.2549)tmp1=0.9;
        else if(FT1<=17.2885)tmp1=1;
    }
    else if(FT1<=17.5921){
        if(FT1==17.2885)tmp1=1;
        else if(FT1<=17.322)tmp1=1.1;
        else if(FT1<=17.3557)tmp1=1.2;
        else if(FT1<=17.3893)tmp1=1.3;
        else if(FT1<=17.423)tmp1=1.4;
        else if(FT1<=17.4568)tmp1=1.5;
        else if(FT1<=17.4905)tmp1=1.6;
        else if(FT1<=17.5244)tmp1=1.7;
        else if(FT1<=17.5582)tmp1=1.8;
        else if(FT1<=17.5921)tmp1=1.9;
    }
    else if(FT1<=17.9335){
        if(FT1==17.6261)tmp1=2;
        else if(FT1<=17.6601)tmp1=2.1;
        else if(FT1<=17.6941)tmp1=2.2;
        else if(FT1<=17.7282)tmp1=2.3;
        else if(FT1<=17.7623)tmp1=2.4;
        else if(FT1<=17.7965)tmp1=2.5;
        else if(FT1<=17.8307)tmp1=2.6;
        else if(FT1<=17.8649)tmp1=2.7;
        else if(FT1<=17.8992)tmp1=2.8;
        else if(FT1<=17.9335)tmp1=2.9;
    }
    else if(FT1<=18.2791){
        if(FT1==17.9679)tmp1=3;
        else if(FT1<=18.0023)tmp1=3.1;
        else if(FT1<=18.0368)tmp1=3.2;
        else if(FT1<=18.0712)tmp1=3.3;
        else if(FT1<=18.1058)tmp1=3.4;
        else if(FT1<=18.1404)tmp1=3.5;
        else if(FT1<=18.175)tmp1=3.6;
        else if(FT1<=18.2096)tmp1=3.7;
        else if(FT1<=18.2444)tmp1=3.8;
        else if(FT1<=18.2791)tmp1=3.9;
    }
    else if(FT1<=18.6289){
        if(FT1==18.3139)tmp1=4;
        else if(FT1<=18.3487)tmp1=4.1;
        else if(FT1<=18.3836)tmp1=4.2;
        else if(FT1<=18.4185)tmp1=4.3;
        else if(FT1<=18.4535)tmp1=4.4;
        else if(FT1<=18.4885)tmp1=4.5;
        else if(FT1<=18.5235)tmp1=4.6;
        else if(FT1<=18.5586)tmp1=4.7;
        else if(FT1<=18.5937)tmp1=4.8;
        else if(FT1<=18.6289)tmp1=4.9;
        else if(FT1<=18.6641)tmp1=5;
    }
    else if(FT1<=18.9829){
        if(FT1==18.6641)tmp1=5;
        else if(FT1<=18.6994)tmp1=5.1;
        else if(FT1<=18.7347)tmp1=5.2;
        else if(FT1<=18.77)tmp1=5.3;
        else if(FT1<=18.8054)tmp1=5.4;
        else if(FT1<=18.8408)tmp1=5.5;
        else if(FT1<=18.8763)tmp1=5.6;
        else if(FT1<=18.9118)tmp1=5.7;
        else if(FT1<=18.9473)tmp1=5.8;
        else if(FT1<=18.9829)tmp1=5.9;
        else if(FT1<=19.0186)tmp1=6;
    }
    else if(FT1<=19.3412){
        if(FT1==19.0186)tmp1=6;
        else if(FT1<=19.0543)tmp1=6.1;
        else if(FT1<=19.09)tmp1=6.2;
        else if(FT1<=19.1257)tmp1=6.3;
        else if(FT1<=19.1615)tmp1=6.4;
        else if(FT1<=19.1974)tmp1=6.5;
        else if(FT1<=19.2333)tmp1=6.6;
        else if(FT1<=19.2692)tmp1=6.7;
        else if(FT1<=19.3052)tmp1=6.8;
        else if(FT1<=19.3412)tmp1=6.9;
        else if(FT1<=19.3773)tmp1=7;
    }
    else if(FT1<=19.7038){
        if(FT1==19.3773)tmp1=7;
        else if(FT1<=19.4134)tmp1=7.1;
        else if(FT1<=19.4495)tmp1=7.2;
        else if(FT1<=19.4857)tmp1=7.3;
        else if(FT1<=19.522)tmp1=7.4;
        else if(FT1<=19.5582)tmp1=7.5;
        else if(FT1<=19.5945)tmp1=7.6;
        else if(FT1<=19.6309)tmp1=7.7;
        else if(FT1<=19.6673)tmp1=7.8;
        else if(FT1<=19.7038)tmp1=7.9;
        else if(FT1<=19.7402)tmp1=8;
    }
    else if(FT1<=20.0706){
        if(FT1==19.7402)tmp1=8;
        else if(FT1<=19.7768)tmp1=8.1;
        else if(FT1<=19.8133)tmp1=8.2;
        else if(FT1<=19.85)tmp1=8.3;
        else if(FT1<=19.8866)tmp1=8.4;
        else if(FT1<=19.9233)tmp1=8.5;
        else if(FT1<=19.9601)tmp1=8.6;
        else if(FT1<=19.9969)tmp1=8.7;
        else if(FT1<=20.0337)tmp1=8.8;
        else if(FT1<=20.0706)tmp1=8.9;
        else if(FT1<=20.1075)tmp1=9;
    }
    else if(FT1<=20.4417){
        if(FT1==20.1075)tmp1=9;
        else if(FT1<=20.1444)tmp1=9.1;
        else if(FT1<=20.1814)tmp1=9.2;
        else if(FT1<=20.2185)tmp1=9.3;
        else if(FT1<=20.2556)tmp1=9.4;
        else if(FT1<=20.2927)tmp1=9.5;
        else if(FT1<=20.3299)tmp1=9.6;
        else if(FT1<=20.3671)tmp1=9.7;
        else if(FT1<=20.4044)tmp1=9.8;
        else if(FT1<=20.4417)tmp1=9.9;
        else if(FT1<=20.479)tmp1=10;
    }
    else if(FT1<=20.817){
        if(FT1==20.479)tmp1=10;
        else if(FT1<=20.5164)tmp1=10.1;
        else if(FT1<=20.5538)tmp1=10.2;
        else if(FT1<=20.5913)tmp1=10.3;
        else if(FT1<=20.6288)tmp1=10.4;
        else if(FT1<=20.6664)tmp1=10.5;
        else if(FT1<=20.704)tmp1=10.6;
        else if(FT1<=20.7416)tmp1=10.7;
        else if(FT1<=20.7793)tmp1=10.8;
        else if(FT1<=20.817)tmp1=10.9;
        else if(FT1<=20.8548)tmp1=11;
    }
    else if(FT1<=21.1967){
        if(FT1==20.8548)tmp1=11;
        else if(FT1<=20.8926)tmp1=11.1;
        else if(FT1<=20.9305)tmp1=11.2;
        else if(FT1<=20.9684)tmp1=11.3;
        else if(FT1<=21.0064)tmp1=11.4;
        else if(FT1<=21.0444)tmp1=11.5;
        else if(FT1<=21.0824)tmp1=11.6;
        else if(FT1<=21.1205)tmp1=11.7;
        else if(FT1<=21.1586)tmp1=11.8;
        else if(FT1<=21.1967)tmp1=11.9;
        else if(FT1<=21.235)tmp1=12;
    }
    else if(FT1<=21.5808){
        if(FT1==21.235)tmp1=12;
        else if(FT1<=21.2732)tmp1=12.1;
        else if(FT1<=21.3115)tmp1=12.2;
        else if(FT1<=21.3498)tmp1=12.3;
        else if(FT1<=21.3882)tmp1=12.4;
        else if(FT1<=21.4266)tmp1=12.5;
        else if(FT1<=21.4651)tmp1=12.6;
        else if(FT1<=21.5036)tmp1=12.7;
        else if(FT1<=21.5422)tmp1=12.8;
        else if(FT1<=21.5808)tmp1=12.9;
        else if(FT1<=21.6194)tmp1=13;
    }
    else if(FT1<=21.9691){
        if(FT1==21.6194)tmp1=13;
        else if(FT1<=21.6581)tmp1=13.1;
        else if(FT1<=21.6968)tmp1=13.2;
        else if(FT1<=21.7356)tmp1=13.3;
        else if(FT1<=21.7744)tmp1=13.4;
        else if(FT1<=21.8133)tmp1=13.5;
        else if(FT1<=21.8522)tmp1=13.6;
        else if(FT1<=21.8911)tmp1=13.7;
        else if(FT1<=21.9301)tmp1=13.8;
        else if(FT1<=21.9691)tmp1=13.9;
        else if(FT1<=22.0082)tmp1=14;
    }
    else if(FT1<=22.3618){
        if(FT1==22.0082)tmp1=14;
        else if(FT1<=22.0473)tmp1=14.1;
        else if(FT1<=22.0865)tmp1=14.2;
        else if(FT1<=22.1257)tmp1=14.3;
        else if(FT1<=22.1649)tmp1=14.4;
        else if(FT1<=22.2042)tmp1=14.5;
        else if(FT1<=22.2436)tmp1=14.6;
        else if(FT1<=22.283)tmp1=14.7;
        else if(FT1<=22.3224)tmp1=14.8;
        else if(FT1<=22.3618)tmp1=14.9;
        else if(FT1<=22.4014)tmp1=15;
    }
    else if(FT1<=22.7589){
        if(FT1==22.4014)tmp1=15;
        else if(FT1<=22.4409)tmp1=15.1;
        else if(FT1<=22.4805)tmp1=15.2;
        else if(FT1<=22.5201)tmp1=15.3;
        else if(FT1<=22.5598)tmp1=15.4;
        else if(FT1<=22.5996)tmp1=15.5;
        else if(FT1<=22.6393)tmp1=15.6;
        else if(FT1<=22.6791)tmp1=15.7;
        else if(FT1<=22.719)tmp1=15.8;
        else if(FT1<=22.7589)tmp1=15.9;
        else if(FT1<=22.7989)tmp1=16;
    }
    else if(FT1<=23.1603){
        if(FT1==22.7989)tmp1=16;
        else if(FT1<=22.8388)tmp1=16.1;
        else if(FT1<=22.8789)tmp1=16.2;
        else if(FT1<=22.919)tmp1=16.3;
        else if(FT1<=22.9591)tmp1=16.4;
        else if(FT1<=22.9992)tmp1=16.5;
        else if(FT1<=23.0394)tmp1=16.6;
        else if(FT1<=23.0797)tmp1=16.7;
        else if(FT1<=23.12)tmp1=16.8;
        else if(FT1<=23.1603)tmp1=16.9;
    }
    else if(FT1<=23.5662){
        if(FT1==23.2007)tmp1=17;
        else if(FT1<=23.2411)tmp1=17.1;
        else if(FT1<=23.2816)tmp1=17.2;
        else if(FT1<=23.3221)tmp1=17.3;
        else if(FT1<=23.3627)tmp1=17.4;
        else if(FT1<=23.4033)tmp1=17.5;
        else if(FT1<=23.4439)tmp1=17.6;
        else if(FT1<=23.4846)tmp1=17.7;
        else if(FT1<=23.5254)tmp1=17.8;
        else if(FT1<=23.5662)tmp1=17.9;
    }
    else if(FT1<=23.9764){
        if(FT1==23.607)tmp1=18;
        else if(FT1<=23.6478)tmp1=18.1;
        else if(FT1<=23.6888)tmp1=18.2;
        else if(FT1<=23.7297)tmp1=18.3;
        else if(FT1<=23.7707)tmp1=18.4;
        else if(FT1<=23.8117)tmp1=18.5;
        else if(FT1<=23.8528)tmp1=18.6;
        else if(FT1<=23.894)tmp1=18.7;
        else if(FT1<=23.9351)tmp1=18.8;
        else if(FT1<=23.9764)tmp1=18.9;
    }
    else if(FT1<=24.391){
        if(FT1==24.0176)tmp1=19;
        else if(FT1<=24.0589)tmp1=19.1;
        else if(FT1<=24.1003)tmp1=19.2;
        else if(FT1<=24.1417)tmp1=19.3;
        else if(FT1<=24.1831)tmp1=19.4;
        else if(FT1<=24.2246)tmp1=19.5;
        else if(FT1<=24.2661)tmp1=19.6;
        else if(FT1<=24.3077)tmp1=19.7;
        else if(FT1<=24.3493)tmp1=19.8;
        else if(FT1<=24.391)tmp1=19.9;
    }
    else if(FT1<=24.81){
        if(FT1==24.4327)tmp1=20;
        else if(FT1<=24.4744)tmp1=20.1;
        else if(FT1<=24.5162)tmp1=20.2;
        else if(FT1<=24.558)tmp1=20.3;
        else if(FT1<=24.5999)tmp1=20.4;
        else if(FT1<=24.6418)tmp1=20.5;
        else if(FT1<=24.6838)tmp1=20.6;
        else if(FT1<=24.7258)tmp1=20.7;
        else if(FT1<=24.7679)tmp1=20.8;
        else if(FT1<=24.81)tmp1=20.9;
    }
    else if(FT1<=25.2334){
        if(FT1==24.8521)tmp1=21;
        else if(FT1<=24.8943)tmp1=21.1;
        else if(FT1<=24.9365)tmp1=21.2;
        else if(FT1<=24.9788)tmp1=21.3;
        else if(FT1<=25.0211)tmp1=21.4;
        else if(FT1<=25.0635)tmp1=21.5;
        else if(FT1<=25.1059)tmp1=21.6;
        else if(FT1<=25.1484)tmp1=21.7;
        else if(FT1<=25.1909)tmp1=21.8;
        else if(FT1<=25.2334)tmp1=21.9;
    }
    else if(FT1<=25.6613){
        if(FT1==25.276)tmp1=22;
        else if(FT1<=25.3186)tmp1=22.1;
        else if(FT1<=25.3613)tmp1=22.2;
        else if(FT1<=25.404)tmp1=22.3;
        else if(FT1<=25.4468)tmp1=22.4;
        else if(FT1<=25.4896)tmp1=22.5;
        else if(FT1<=25.5324)tmp1=22.6;
        else if(FT1<=25.5753)tmp1=22.7;
        else if(FT1<=25.6183)tmp1=22.8;
        else if(FT1<=25.6613)tmp1=22.9;
    }
    else if(FT1<=26.0936){
        if(FT1==25.7043)tmp1=23;
        else if(FT1<=25.7474)tmp1=23.1;
        else if(FT1<=25.7905)tmp1=23.2;
        else if(FT1<=25.8337)tmp1=23.3;
        else if(FT1<=25.8769)tmp1=23.4;
        else if(FT1<=25.9201)tmp1=23.5;
        else if(FT1<=25.9634)tmp1=23.6;
        else if(FT1<=26.0068)tmp1=23.7;
        else if(FT1<=26.0501)tmp1=23.8;
        else if(FT1<=26.0936)tmp1=23.9;
    }
    else if(FT1<=26.5303){
        if(FT1==26.137)tmp1=24;
        else if(FT1<=26.1806)tmp1=24.1;
        else if(FT1<=26.2241)tmp1=24.2;
        else if(FT1<=26.2677)tmp1=24.3;
        else if(FT1<=26.3114)tmp1=24.4;
        else if(FT1<=26.3551)tmp1=24.5;
        else if(FT1<=26.3988)tmp1=24.6;
        else if(FT1<=26.4426)tmp1=24.7;
        else if(FT1<=26.4865)tmp1=24.8;
        else if(FT1<=26.5303)tmp1=24.9;
    }
    else if(FT1<=26.9715){
        if(FT1==26.5742)tmp1=25;
        else if(FT1<=26.6182)tmp1=25.1;
        else if(FT1<=26.6622)tmp1=25.2;
        else if(FT1<=26.7063)tmp1=25.3;
        else if(FT1<=26.7504)tmp1=25.4;
        else if(FT1<=26.7945)tmp1=25.5;
        else if(FT1<=26.8387)tmp1=25.6;
        else if(FT1<=26.8829)tmp1=25.7;
        else if(FT1<=26.9272)tmp1=25.8;
        else if(FT1<=26.9715)tmp1=25.9;
    }
    else if(FT1<=27.4172){
        if(FT1==27.0159)tmp1=26;
        else if(FT1<=27.0603)tmp1=26.1;
        else if(FT1<=27.1048)tmp1=26.2;
        else if(FT1<=27.1493)tmp1=26.3;
        else if(FT1<=27.1938)tmp1=26.4;
        else if(FT1<=27.2384)tmp1=26.5;
        else if(FT1<=27.283)tmp1=26.6;
        else if(FT1<=27.3277)tmp1=26.7;
        else if(FT1<=27.3724)tmp1=26.8;
        else if(FT1<=27.4172)tmp1=26.9;
    }
    else if(FT1<=27.8674){
        if(FT1==27.462)tmp1=27;
        else if(FT1<=27.5069)tmp1=27.1;
        else if(FT1<=27.5518)tmp1=27.2;
        else if(FT1<=27.5967)tmp1=27.3;
        else if(FT1<=27.6417)tmp1=27.4;
        else if(FT1<=27.6868)tmp1=27.5;
        else if(FT1<=27.7318)tmp1=27.6;
        else if(FT1<=27.777)tmp1=27.7;
        else if(FT1<=27.8221)tmp1=27.8;
        else if(FT1<=27.8674)tmp1=27.9;
    }
    else if(FT1<=28.322){
        if(FT1==27.9126)tmp1=28;
        else if(FT1<=27.9579)tmp1=28.1;
        else if(FT1<=28.0033)tmp1=28.2;
        else if(FT1<=28.0487)tmp1=28.3;
        else if(FT1<=28.0941)tmp1=28.4;
        else if(FT1<=28.1396)tmp1=28.5;
        else if(FT1<=28.1851)tmp1=28.6;
        else if(FT1<=28.2307)tmp1=28.7;
        else if(FT1<=28.2763)tmp1=28.8;
        else if(FT1<=28.322)tmp1=28.9;
    }
    else if(FT1<=28.7811){
        if(FT1==28.3677)tmp1=29;
        else if(FT1<=28.4134)tmp1=29.1;
        else if(FT1<=28.4592)tmp1=29.2;
        else if(FT1<=28.5051)tmp1=29.3;
        else if(FT1<=28.551)tmp1=29.4;
        else if(FT1<=28.5969)tmp1=29.5;
        else if(FT1<=28.6429)tmp1=29.6;
        else if(FT1<=28.6889)tmp1=29.7;
        else if(FT1<=28.735)tmp1=29.8;
        else if(FT1<=28.7811)tmp1=29.9;
    }
    else if(FT1<=29.2447){
        if(FT1==28.8273)tmp1=30;
        else if(FT1<=28.8735)tmp1=30.1;
        else if(FT1<=28.9197)tmp1=30.2;
        else if(FT1<=28.966)tmp1=30.3;
        else if(FT1<=29.0123)tmp1=30.4;
        else if(FT1<=29.0587)tmp1=30.5;
        else if(FT1<=29.1052)tmp1=30.6;
        else if(FT1<=29.1516)tmp1=30.7;
        else if(FT1<=29.1981)tmp1=30.8;
        else if(FT1<=29.2447)tmp1=30.9;
    }
    else if(FT1<=29.7128){
        if(FT1==29.2913)tmp1=31;
        else if(FT1<=29.338)tmp1=31.1;
        else if(FT1<=29.3847)tmp1=31.2;
        else if(FT1<=29.4314)tmp1=31.3;
        else if(FT1<=29.4782)tmp1=31.4;
        else if(FT1<=29.525)tmp1=31.5;
        else if(FT1<=29.5719)tmp1=31.6;
        else if(FT1<=29.6188)tmp1=31.7;
        else if(FT1<=29.6658)tmp1=31.8;
        else if(FT1<=29.7128)tmp1=31.9;
    }
    else if(FT1<=30.1855){
        if(FT1==29.7599)tmp1=32;
        else if(FT1<=29.807)tmp1=32.1;
        else if(FT1<=29.8541)tmp1=32.2;
        else if(FT1<=29.9013)tmp1=32.3;
        else if(FT1<=29.9486)tmp1=32.4;
        else if(FT1<=29.9959)tmp1=32.5;
        else if(FT1<=30.0432)tmp1=32.6;
        else if(FT1<=30.0906)tmp1=32.7;
        else if(FT1<=30.138)tmp1=32.8;
        else if(FT1<=30.1855)tmp1=32.9;
    }
    else if(FT1<=30.6626){
        if(FT1==30.233)tmp1=33;
        else if(FT1<=30.2805)tmp1=33.1;
        else if(FT1<=30.3281)tmp1=33.2;
        else if(FT1<=30.3758)tmp1=33.3;
        else if(FT1<=30.4235)tmp1=33.4;
        else if(FT1<=30.4712)tmp1=33.5;
        else if(FT1<=30.519)tmp1=33.6;
        else if(FT1<=30.5668)tmp1=33.7;
        else if(FT1<=30.6147)tmp1=33.8;
        else if(FT1<=30.6626)tmp1=33.9;
    }
    else if(FT1<=31.1443){
        if(FT1==30.7106)tmp1=34;
        else if(FT1<=30.7586)tmp1=34.1;
        else if(FT1<=30.8066)tmp1=34.2;
        else if(FT1<=30.8547)tmp1=34.3;
        else if(FT1<=30.9029)tmp1=34.4;
        else if(FT1<=30.9511)tmp1=34.5;
        else if(FT1<=30.9993)tmp1=34.6;
        else if(FT1<=31.0476)tmp1=34.7;
        else if(FT1<=31.0959)tmp1=34.8;
        else if(FT1<=31.1443)tmp1=34.9;
    }
    else if(FT1<=31.6305){
        if(FT1==31.1927)tmp1=35;
        else if(FT1<=31.2411)tmp1=35.1;
        else if(FT1<=31.2897)tmp1=35.2;
        else if(FT1<=31.3382)tmp1=35.3;
        else if(FT1<=31.3868)tmp1=35.4;
        else if(FT1<=31.4354)tmp1=35.5;
        else if(FT1<=31.4841)tmp1=35.6;
        else if(FT1<=31.5329)tmp1=35.7;
        else if(FT1<=31.5816)tmp1=35.8;
        else if(FT1<=31.6305)tmp1=35.9;
    }
    else if(FT1<=32.1212){
        if(FT1==31.6793)tmp1=36;
        else if(FT1<=31.7283)tmp1=36.1;
        else if(FT1<=31.7772)tmp1=36.2;
        else if(FT1<=31.8262)tmp1=36.3;
        else if(FT1<=31.8753)tmp1=36.4;
        else if(FT1<=31.9244)tmp1=36.5;
        else if(FT1<=31.9735)tmp1=36.6;
        else if(FT1<=32.0227)tmp1=36.7;
        else if(FT1<=32.0719)tmp1=36.8;
        else if(FT1<=32.1212)tmp1=36.9;
    }
    else if(FT1<=32.6165){
        if(FT1==32.1705)tmp1=37;
        else if(FT1<=32.2199)tmp1=37.1;
        else if(FT1<=32.2693)tmp1=37.2;
        else if(FT1<=32.3188)tmp1=37.3;
        else if(FT1<=32.3683)tmp1=37.4;
        else if(FT1<=32.4178)tmp1=37.5;
        else if(FT1<=32.4674)tmp1=37.6;
        else if(FT1<=32.5171)tmp1=37.7;
        else if(FT1<=32.5667)tmp1=37.8;
        else if(FT1<=32.6165)tmp1=37.9;
    }
    else if(FT1<=33.1163){
        if(FT1==32.6663)tmp1=38;
        else if(FT1<=32.7161)tmp1=38.1;
        else if(FT1<=32.7659)tmp1=38.2;
        else if(FT1<=32.8159)tmp1=38.3;
        else if(FT1<=32.8658)tmp1=38.4;
        else if(FT1<=32.9158)tmp1=38.5;
        else if(FT1<=32.9659)tmp1=38.6;
        else if(FT1<=33.016)tmp1=38.7;
        else if(FT1<=33.0661)tmp1=38.8;
        else if(FT1<=33.1163)tmp1=38.9;
    }
    else if(FT1<=33.6207){
        if(FT1==33.1665)tmp1=39;
        else if(FT1<=33.2168)tmp1=39.1;
        else if(FT1<=33.2671)tmp1=39.2;
        else if(FT1<=33.3175)tmp1=39.3;
        else if(FT1<=33.3679)tmp1=39.4;
        else if(FT1<=33.4184)tmp1=39.5;
        else if(FT1<=33.4689)tmp1=39.6;
        else if(FT1<=33.5194)tmp1=39.7;
        else if(FT1<=33.57)tmp1=39.8;
        else if(FT1<=33.6207)tmp1=39.9;
    }
    else if(FT1<=34.1296){
        if(FT1==33.6714)tmp1=40;
        else if(FT1<=33.7221)tmp1=40.1;
        else if(FT1<=33.7729)tmp1=40.2;
        else if(FT1<=33.8237)tmp1=40.3;
        else if(FT1<=33.8746)tmp1=40.4;
        else if(FT1<=33.9255)tmp1=40.5;
        else if(FT1<=33.9764)tmp1=40.6;
        else if(FT1<=34.0275)tmp1=40.7;
        else if(FT1<=34.0785)tmp1=40.8;
        else if(FT1<=34.1296)tmp1=40.9;
    }
    else if(FT1<=34.6431){
        if(FT1==34.1807)tmp1=41;
        else if(FT1<=34.2319)tmp1=41.1;
        else if(FT1<=34.2832)tmp1=41.2;
        else if(FT1<=34.3345)tmp1=41.3;
        else if(FT1<=34.3858)tmp1=41.4;
        else if(FT1<=34.4372)tmp1=41.5;
        else if(FT1<=34.4886)tmp1=41.6;
        else if(FT1<=34.54)tmp1=41.7;
        else if(FT1<=34.5915)tmp1=41.8;
        else if(FT1<=34.6431)tmp1=41.9;
    }
    else if(FT1<=35.1612){
        if(FT1==34.6947)tmp1=42;
        else if(FT1<=34.7463)tmp1=42.1;
        else if(FT1<=34.798)tmp1=42.2;
        else if(FT1<=34.8498)tmp1=42.3;
        else if(FT1<=34.9016)tmp1=42.4;
        else if(FT1<=34.9534)tmp1=42.5;
        else if(FT1<=35.0053)tmp1=42.6;
        else if(FT1<=35.0572)tmp1=42.7;
        else if(FT1<=35.1091)tmp1=42.8;
        else if(FT1<=35.1612)tmp1=42.9;
    }
    else if(FT1<=35.6838){
        if(FT1==35.2132)tmp1=43;
        else if(FT1<=35.2653)tmp1=43.1;
        else if(FT1<=35.3175)tmp1=43.2;
        else if(FT1<=35.3697)tmp1=43.3;
        else if(FT1<=35.4219)tmp1=43.4;
        else if(FT1<=35.4742)tmp1=43.5;
        else if(FT1<=35.5265)tmp1=43.6;
        else if(FT1<=35.5789)tmp1=43.7;
        else if(FT1<=35.6313)tmp1=43.8;
        else if(FT1<=35.6838)tmp1=43.9;
    }
    else if(FT1<=36.211){
        if(FT1==35.7363)tmp1=44;
        else if(FT1<=35.7889)tmp1=44.1;
        else if(FT1<=35.8415)tmp1=44.2;
        else if(FT1<=35.8941)tmp1=44.3;
        else if(FT1<=35.9468)tmp1=44.4;
        else if(FT1<=35.9996)tmp1=44.5;
        else if(FT1<=36.0523)tmp1=44.6;
        else if(FT1<=36.1052)tmp1=44.7;
        else if(FT1<=36.1581)tmp1=44.8;
        else if(FT1<=36.211)tmp1=44.9;
    }
    else if(FT1<=36.7428){
        if(FT1==36.264)tmp1=45;
        else if(FT1<=36.317)tmp1=45.1;
        else if(FT1<=36.37)tmp1=45.2;
        else if(FT1<=36.4231)tmp1=45.3;
        else if(FT1<=36.4763)tmp1=45.4;
        else if(FT1<=36.5295)tmp1=45.5;
        else if(FT1<=36.5828)tmp1=45.6;
        else if(FT1<=36.6361)tmp1=45.7;
        else if(FT1<=36.6894)tmp1=45.8;
        else if(FT1<=36.7428)tmp1=45.9;
    }
    else if(FT1<=37.2791){
        if(FT1==36.7962)tmp1=46;
        else if(FT1<=36.8497)tmp1=46.1;
        else if(FT1<=36.9032)tmp1=46.2;
        else if(FT1<=36.9568)tmp1=46.3;
        else if(FT1<=37.0104)tmp1=46.4;
        else if(FT1<=37.064)tmp1=46.5;
        else if(FT1<=37.1178)tmp1=46.6;
        else if(FT1<=37.1715)tmp1=46.7;
        else if(FT1<=37.2253)tmp1=46.8;
        else if(FT1<=37.2791)tmp1=46.9;
    }
    else if(FT1<=37.8201){
        if(FT1==37.333)tmp1=47;
        else if(FT1<=37.387)tmp1=47.1;
        else if(FT1<=37.4409)tmp1=47.2;
        else if(FT1<=37.495)tmp1=47.3;
        else if(FT1<=37.549)tmp1=47.4;
        else if(FT1<=37.6032)tmp1=47.5;
        else if(FT1<=37.6573)tmp1=47.6;
        else if(FT1<=37.7115)tmp1=47.7;
        else if(FT1<=37.7658)tmp1=47.8;
        else if(FT1<=37.8201)tmp1=47.9;
    }
    else if(FT1<=38.3656){
        if(FT1==37.8744)tmp1=48;
        else if(FT1<=37.9288)tmp1=48.1;
        else if(FT1<=37.9833)tmp1=48.2;
        else if(FT1<=38.0378)tmp1=48.3;
        else if(FT1<=38.0923)tmp1=48.4;
        else if(FT1<=38.1469)tmp1=48.5;
        else if(FT1<=38.2015)tmp1=48.6;
        else if(FT1<=38.2562)tmp1=48.7;
        else if(FT1<=38.3109)tmp1=48.8;
        else if(FT1<=38.3656)tmp1=48.9;
    }
    else if(FT1<=38.9158){
        if(FT1==38.4204)tmp1=49;
        else if(FT1<=38.4753)tmp1=49.1;
        else if(FT1<=38.5302)tmp1=49.2;
        else if(FT1<=38.5851)tmp1=49.3;
        else if(FT1<=38.6401)tmp1=49.4;
        else if(FT1<=38.6952)tmp1=49.5;
        else if(FT1<=38.7502)tmp1=49.6;
        else if(FT1<=38.8054)tmp1=49.7;
        else if(FT1<=38.8605)tmp1=49.8;
        else if(FT1<=38.9158)tmp1=49.9;
    }
    else if(FT1<=39.4705){
        if(FT1==38.971)tmp1=50;
        else if(FT1<=39.0263)tmp1=50.1;
        else if(FT1<=39.0817)tmp1=50.2;
        else if(FT1<=39.1371)tmp1=50.3;
        else if(FT1<=39.1926)tmp1=50.4;
        else if(FT1<=39.2481)tmp1=50.5;
        else if(FT1<=39.3036)tmp1=50.6;
        else if(FT1<=39.3592)tmp1=50.7;
        else if(FT1<=39.4148)tmp1=50.8;
        else if(FT1<=39.4705)tmp1=50.9;
    }
    else if(FT1<=40.0298){
        if(FT1==39.5262)tmp1=51;
        else if(FT1<=39.582)tmp1=51.1;
        else if(FT1<=39.6378)tmp1=51.2;
        else if(FT1<=39.6937)tmp1=51.3;
        else if(FT1<=39.7496)tmp1=51.4;
        else if(FT1<=39.8055)tmp1=51.5;
        else if(FT1<=39.8615)tmp1=51.6;
        else if(FT1<=39.9176)tmp1=51.7;
        else if(FT1<=39.9737)tmp1=51.8;
        else if(FT1<=40.0298)tmp1=51.9;
    }
    else if(FT1<=40.5937){
        if(FT1==40.086)tmp1=52;
        else if(FT1<=40.1422)tmp1=52.1;
        else if(FT1<=40.1985)tmp1=52.2;
        else if(FT1<=40.2548)tmp1=52.3;
        else if(FT1<=40.3112)tmp1=52.4;
        else if(FT1<=40.3676)tmp1=52.5;
        else if(FT1<=40.4241)tmp1=52.6;
        else if(FT1<=40.4806)tmp1=52.7;
        else if(FT1<=40.5371)tmp1=52.8;
        else if(FT1<=40.5937)tmp1=52.9;
    }
    else if(FT1<=41.1623){
        if(FT1==40.6504)tmp1=53;
        else if(FT1<=40.7071)tmp1=53.1;
        else if(FT1<=40.7638)tmp1=53.2;
        else if(FT1<=40.8206)tmp1=53.3;
        else if(FT1<=40.8774)tmp1=53.4;
        else if(FT1<=40.9343)tmp1=53.5;
        else if(FT1<=40.9912)tmp1=53.6;
        else if(FT1<=41.0482)tmp1=53.7;
        else if(FT1<=41.1052)tmp1=53.8;
        else if(FT1<=41.1623)tmp1=53.9;
    }
    else if(FT1<=41.7354){
        if(FT1==41.2194)tmp1=54;
        else if(FT1<=41.2765)tmp1=54.1;
        else if(FT1<=41.3337)tmp1=54.2;
        else if(FT1<=41.391)tmp1=54.3;
        else if(FT1<=41.4483)tmp1=54.4;
        else if(FT1<=41.5056)tmp1=54.5;
        else if(FT1<=41.563)tmp1=54.6;
        else if(FT1<=41.6204)tmp1=54.7;
        else if(FT1<=41.6779)tmp1=54.8;
        else if(FT1<=41.7354)tmp1=54.9;
    }
    else if(FT1<=42.3131){
        if(FT1==41.793)tmp1=55;
        else if(FT1<=41.8506)tmp1=55.1;
        else if(FT1<=41.9082)tmp1=55.2;
        else if(FT1<=41.9659)tmp1=55.3;
        else if(FT1<=42.0237)tmp1=55.4;
        else if(FT1<=42.0815)tmp1=55.5;
        else if(FT1<=42.1393)tmp1=55.6;
        else if(FT1<=42.1972)tmp1=55.7;
        else if(FT1<=42.2552)tmp1=55.8;
        else if(FT1<=42.3131)tmp1=55.9;
    }
    else if(FT1<=42.8955){
        if(FT1==42.3712)tmp1=56;
        else if(FT1<=42.4292)tmp1=56.1;
        else if(FT1<=42.4874)tmp1=56.2;
        else if(FT1<=42.5455)tmp1=56.3;
        else if(FT1<=42.6037)tmp1=56.4;
        else if(FT1<=42.662)tmp1=56.5;
        else if(FT1<=42.7203)tmp1=56.6;
        else if(FT1<=42.7787)tmp1=56.7;
        else if(FT1<=42.837)tmp1=56.8;
        else if(FT1<=42.8955)tmp1=56.9;
    }
    else if(FT1<=43.4824){
        if(FT1==42.954)tmp1=57;
        else if(FT1<=43.0125)tmp1=57.1;
        else if(FT1<=43.0711)tmp1=57.2;
        else if(FT1<=43.1297)tmp1=57.3;
        else if(FT1<=43.1884)tmp1=57.4;
        else if(FT1<=43.2471)tmp1=57.5;
        else if(FT1<=43.3059)tmp1=57.6;
        else if(FT1<=43.3647)tmp1=57.7;
        else if(FT1<=43.4235)tmp1=57.8;
        else if(FT1<=43.4824)tmp1=57.9;
    }
    else if(FT1<=44.074){
        if(FT1==43.5414)tmp1=58;
        else if(FT1<=43.6004)tmp1=58.1;
        else if(FT1<=43.6594)tmp1=58.2;
        else if(FT1<=43.7185)tmp1=58.3;
        else if(FT1<=43.7777)tmp1=58.4;
        else if(FT1<=43.8368)tmp1=58.5;
        else if(FT1<=43.8961)tmp1=58.6;
        else if(FT1<=43.9553)tmp1=58.7;
        else if(FT1<=44.0146)tmp1=58.8;
        else if(FT1<=44.074)tmp1=58.9;
    }
    else if(FT1<=44.6702){
        if(FT1==44.1334)tmp1=59;
        else if(FT1<=44.1929)tmp1=59.1;
        else if(FT1<=44.2524)tmp1=59.2;
        else if(FT1<=44.3119)tmp1=59.3;
        else if(FT1<=44.3715)tmp1=59.4;
        else if(FT1<=44.4312)tmp1=59.5;
        else if(FT1<=44.4908)tmp1=59.6;
        else if(FT1<=44.5506)tmp1=59.7;
        else if(FT1<=44.6104)tmp1=59.8;
        else if(FT1<=44.6702)tmp1=59.9;
    }
    else if(FT1<=45.271){
        if(FT1==44.7301)tmp1=60;
        else if(FT1<=44.79)tmp1=60.1;
        else if(FT1<=44.8499)tmp1=60.2;
        else if(FT1<=44.9099)tmp1=60.3;
        else if(FT1<=44.97)tmp1=60.4;
        else if(FT1<=45.0301)tmp1=60.5;
        else if(FT1<=45.0903)tmp1=60.6;
        else if(FT1<=45.1504)tmp1=60.7;
        else if(FT1<=45.2107)tmp1=60.8;
        else if(FT1<=45.271)tmp1=60.9;
    }
    else if(FT1<=45.8764){
        if(FT1==45.3313)tmp1=61;
        else if(FT1<=45.3917)tmp1=61.1;
        else if(FT1<=45.4521)tmp1=61.2;
        else if(FT1<=45.5126)tmp1=61.3;
        else if(FT1<=45.5731)tmp1=61.4;
        else if(FT1<=45.6337)tmp1=61.5;
        else if(FT1<=45.6943)tmp1=61.6;
        else if(FT1<=45.7549)tmp1=61.7;
        else if(FT1<=45.8156)tmp1=61.8;
        else if(FT1<=45.8764)tmp1=61.9;
    }
    else if(FT1<=46.4864){
        if(FT1==45.9372)tmp1=62;
        else if(FT1<=45.998)tmp1=62.1;
        else if(FT1<=46.0589)tmp1=62.2;
        else if(FT1<=46.1198)tmp1=62.3;
        else if(FT1<=46.1808)tmp1=62.4;
        else if(FT1<=46.2418)tmp1=62.5;
        else if(FT1<=46.3029)tmp1=62.6;
        else if(FT1<=46.364)tmp1=62.7;
        else if(FT1<=46.4252)tmp1=62.8;
        else if(FT1<=46.4864)tmp1=62.9;
    }
    else if(FT1<=47.101){
        if(FT1==46.5476)tmp1=63;
        else if(FT1<=46.6089)tmp1=63.1;
        else if(FT1<=46.6703)tmp1=63.2;
        else if(FT1<=46.7317)tmp1=63.3;
        else if(FT1<=46.7931)tmp1=63.4;
        else if(FT1<=46.8546)tmp1=63.5;
        else if(FT1<=46.9161)tmp1=63.6;
        else if(FT1<=46.9777)tmp1=63.7;
        else if(FT1<=47.0393)tmp1=63.8;
        else if(FT1<=47.101)tmp1=63.9;
    }
    else if(FT1<=47.7203){
        if(FT1==47.1627)tmp1=64;
        else if(FT1<=47.2245)tmp1=64.1;
        else if(FT1<=47.2863)tmp1=64.2;
        else if(FT1<=47.3482)tmp1=64.3;
        else if(FT1<=47.4101)tmp1=64.4;
        else if(FT1<=47.472)tmp1=64.5;
        else if(FT1<=47.534)tmp1=64.6;
        else if(FT1<=47.596)tmp1=64.7;
        else if(FT1<=47.6581)tmp1=64.8;
        else if(FT1<=47.7203)tmp1=64.9;
        else if(FT1<=47.7824)tmp1=65;
    }
    else if(FT1<=48.3441){
        if(FT1==47.7824)tmp1=65;
        else if(FT1<=47.8447)tmp1=65.1;
        else if(FT1<=47.9069)tmp1=65.2;
        else if(FT1<=47.9692)tmp1=65.3;
        else if(FT1<=48.0316)tmp1=65.4;
        else if(FT1<=48.094)tmp1=65.5;
        else if(FT1<=48.1565)tmp1=65.6;
        else if(FT1<=48.219)tmp1=65.7;
        else if(FT1<=48.2815)tmp1=65.8;
        else if(FT1<=48.3441)tmp1=65.9;
        else if(FT1<=48.4067)tmp1=66;
    }
    else if(FT1<=48.9726){
        if(FT1==48.4067)tmp1=66;
        else if(FT1<=48.4694)tmp1=66.1;
        else if(FT1<=48.5322)tmp1=66.2;
        else if(FT1<=48.5949)tmp1=66.3;
        else if(FT1<=48.6578)tmp1=66.4;
        else if(FT1<=48.7206)tmp1=66.5;
        else if(FT1<=48.7836)tmp1=66.6;
        else if(FT1<=48.8465)tmp1=66.7;
        else if(FT1<=48.9095)tmp1=66.8;
        else if(FT1<=48.9726)tmp1=66.9;
        else if(FT1<=49.0357)tmp1=67;
    }
    else if(FT1<=49.6057){
        if(FT1==49.0357)tmp1=67;
        else if(FT1<=49.0988)tmp1=67.1;
        else if(FT1<=49.162)tmp1=67.2;
        else if(FT1<=49.2253)tmp1=67.3;
        else if(FT1<=49.2885)tmp1=67.4;
        else if(FT1<=49.3519)tmp1=67.5;
        else if(FT1<=49.4152)tmp1=67.6;
        else if(FT1<=49.4787)tmp1=67.7;
        else if(FT1<=49.5421)tmp1=67.8;
        else if(FT1<=49.6057)tmp1=67.9;
        else if(FT1<=49.6692)tmp1=68;
    }
    else if(FT1<=50.2433){
        if(FT1==49.6692)tmp1=68;
        else if(FT1<=49.7328)tmp1=68.1;
        else if(FT1<=49.7965)tmp1=68.2;
        else if(FT1<=49.8602)tmp1=68.3;
        else if(FT1<=49.9239)tmp1=68.4;
        else if(FT1<=49.9877)tmp1=68.5;
        else if(FT1<=50.0516)tmp1=68.6;
        else if(FT1<=50.1154)tmp1=68.7;
        else if(FT1<=50.1794)tmp1=68.8;
        else if(FT1<=50.2433)tmp1=68.9;
        else if(FT1<=50.3074)tmp1=69;
    }
    else if(FT1<=50.8857){
        if(FT1==50.3074)tmp1=69;
        else if(FT1<=50.3714)tmp1=69.1;
        else if(FT1<=50.4356)tmp1=69.2;
        else if(FT1<=50.4997)tmp1=69.3;
        else if(FT1<=50.5639)tmp1=69.4;
        else if(FT1<=50.6282)tmp1=69.5;
        else if(FT1<=50.6925)tmp1=69.6;
        else if(FT1<=50.7568)tmp1=69.7;
        else if(FT1<=50.8212)tmp1=69.8;
        else if(FT1<=50.8857)tmp1=69.9;
        else if(FT1<=50.9501)tmp1=70;
    }
    else if(FT1<=51.5326){
        if(FT1==50.9501)tmp1=70;
        else if(FT1<=51.0147)tmp1=70.1;
        else if(FT1<=51.0792)tmp1=70.2;
        else if(FT1<=51.1439)tmp1=70.3;
        else if(FT1<=51.2085)tmp1=70.4;
        else if(FT1<=51.2732)tmp1=70.5;
        else if(FT1<=51.338)tmp1=70.6;
        else if(FT1<=51.4028)tmp1=70.7;
        else if(FT1<=51.4677)tmp1=70.8;
        else if(FT1<=51.5326)tmp1=70.9;
        else if(FT1<=51.5975)tmp1=71;
    }
    else if(FT1<=52.1841){
        if(FT1==51.5975)tmp1=71;
        else if(FT1<=51.6625)tmp1=71.1;
        else if(FT1<=51.7275)tmp1=71.2;
        else if(FT1<=51.7926)tmp1=71.3;
        else if(FT1<=51.8578)tmp1=71.4;
        else if(FT1<=51.9229)tmp1=71.5;
        else if(FT1<=51.9882)tmp1=71.6;
        else if(FT1<=52.0534)tmp1=71.7;
        else if(FT1<=52.1187)tmp1=71.8;
        else if(FT1<=52.1841)tmp1=71.9;
        else if(FT1<=52.2495)tmp1=72;
    }
    else if(FT1<=52.8402){
        if(FT1==52.2495)tmp1=72;
        else if(FT1<=52.3149)tmp1=72.1;
        else if(FT1<=52.3804)tmp1=72.2;
        else if(FT1<=52.446)tmp1=72.3;
        else if(FT1<=52.5116)tmp1=72.4;
        else if(FT1<=52.5772)tmp1=72.5;
        else if(FT1<=52.6429)tmp1=72.6;
        else if(FT1<=52.7086)tmp1=72.7;
        else if(FT1<=52.7744)tmp1=72.8;
        else if(FT1<=52.8402)tmp1=72.9;
        else if(FT1<=52.9061)tmp1=73;
    }
    else if(FT1<=53.501){
        if(FT1==52.9061)tmp1=73;
        else if(FT1<=52.972)tmp1=73.1;
        else if(FT1<=53.038)tmp1=73.2;
        else if(FT1<=53.104)tmp1=73.3;
        else if(FT1<=53.17)tmp1=73.4;
        else if(FT1<=53.2361)tmp1=73.5;
        else if(FT1<=53.3023)tmp1=73.6;
        else if(FT1<=53.3684)tmp1=73.7;
        else if(FT1<=53.4347)tmp1=73.8;
        else if(FT1<=53.501)tmp1=73.9;
        else if(FT1<=53.5673)tmp1=74;
    }
    else if(FT1<=54.1663){
        if(FT1==53.5673)tmp1=74;
        else if(FT1<=53.6337)tmp1=74.1;
        else if(FT1<=53.7001)tmp1=74.2;
        else if(FT1<=53.7665)tmp1=74.3;
        else if(FT1<=53.8331)tmp1=74.4;
        else if(FT1<=53.8996)tmp1=74.5;
        else if(FT1<=53.9662)tmp1=74.6;
        else if(FT1<=54.0329)tmp1=74.7;
        else if(FT1<=54.0996)tmp1=74.8;
        else if(FT1<=54.1663)tmp1=74.9;
        else if(FT1<=54.2331)tmp1=75;
    }
    else if(FT1<=54.8363){
        if(FT1==54.2331)tmp1=75;
        else if(FT1<=54.2999)tmp1=75.1;
        else if(FT1<=54.3668)tmp1=75.2;
        else if(FT1<=54.4337)tmp1=75.3;
        else if(FT1<=54.5007)tmp1=75.4;
        else if(FT1<=54.5677)tmp1=75.5;
        else if(FT1<=54.6348)tmp1=75.6;
        else if(FT1<=54.7019)tmp1=75.7;
        else if(FT1<=54.7691)tmp1=75.8;
        else if(FT1<=54.8363)tmp1=75.9;
        else if(FT1<=54.9035)tmp1=76;
    }
    else if(FT1<=55.5108){
        if(FT1==54.9035)tmp1=76;
        else if(FT1<=54.9708)tmp1=76.1;
        else if(FT1<=55.0381)tmp1=76.2;
        else if(FT1<=55.1055)tmp1=76.3;
        else if(FT1<=55.173)tmp1=76.4;
        else if(FT1<=55.2404)tmp1=76.5;
        else if(FT1<=55.308)tmp1=76.6;
        else if(FT1<=55.3755)tmp1=76.7;
        else if(FT1<=55.4431)tmp1=76.8;
        else if(FT1<=55.5108)tmp1=76.9;
        else if(FT1<=55.5785)tmp1=77;
    }
    else if(FT1<=56.19){
        if(FT1==55.5785)tmp1=77;
        else if(FT1<=55.6463)tmp1=77.1;
        else if(FT1<=55.7141)tmp1=77.2;
        else if(FT1<=55.7819)tmp1=77.3;
        else if(FT1<=55.8498)tmp1=77.4;
        else if(FT1<=55.9177)tmp1=77.5;
        else if(FT1<=55.9857)tmp1=77.6;
        else if(FT1<=56.0538)tmp1=77.7;
        else if(FT1<=56.1218)tmp1=77.8;
        else if(FT1<=56.19)tmp1=77.9;
        else if(FT1<=56.2581)tmp1=78;
    }
    else if(FT1<=56.8737){
        if(FT1==56.2581)tmp1=78;
        else if(FT1<=56.3263)tmp1=78.1;
        else if(FT1<=56.3946)tmp1=78.2;
        else if(FT1<=56.4629)tmp1=78.3;
        else if(FT1<=56.5313)tmp1=78.4;
        else if(FT1<=56.5996)tmp1=78.5;
        else if(FT1<=56.6681)tmp1=78.6;
        else if(FT1<=56.7366)tmp1=78.7;
        else if(FT1<=56.8051)tmp1=78.8;
        else if(FT1<=56.8737)tmp1=78.9;
        else if(FT1<=56.9423)tmp1=79;
    }
    else if(FT1<=57.562){
        if(FT1==56.9423)tmp1=79;
        else if(FT1<=57.011)tmp1=79.1;
        else if(FT1<=57.0797)tmp1=79.2;
        else if(FT1<=57.1485)tmp1=79.3;
        else if(FT1<=57.2173)tmp1=79.4;
        else if(FT1<=57.2862)tmp1=79.5;
        else if(FT1<=57.3551)tmp1=79.6;
        else if(FT1<=57.424)tmp1=79.7;
        else if(FT1<=57.493)tmp1=79.8;
        else if(FT1<=57.562)tmp1=79.9;
        else if(FT1<=57.6311)tmp1=80;
    }
    else if(FT1<=58.255){
        if(FT1==57.6311)tmp1=80;
        else if(FT1<=57.7003)tmp1=80.1;
        else if(FT1<=57.7694)tmp1=80.2;
        else if(FT1<=57.8387)tmp1=80.3;
        else if(FT1<=57.9079)tmp1=80.4;
        else if(FT1<=57.9772)tmp1=80.5;
        else if(FT1<=58.0466)tmp1=80.6;
        else if(FT1<=58.116)tmp1=80.7;
        else if(FT1<=58.1855)tmp1=80.8;
        else if(FT1<=58.255)tmp1=80.9;
        else if(FT1<=58.3245)tmp1=81;
    }
    else if(FT1<=58.9525){
        if(FT1==58.3245)tmp1=81;
        else if(FT1<=58.3941)tmp1=81.1;
        else if(FT1<=58.4637)tmp1=81.2;
        else if(FT1<=58.5334)tmp1=81.3;
        else if(FT1<=58.6032)tmp1=81.4;
        else if(FT1<=58.6729)tmp1=81.5;
        else if(FT1<=58.7428)tmp1=81.6;
        else if(FT1<=58.8126)tmp1=81.7;
        else if(FT1<=58.8825)tmp1=81.8;
        else if(FT1<=58.9525)tmp1=81.9;
        else if(FT1<=59.0225)tmp1=82;
    }
    else if(FT1<=59.6546){
        if(FT1==59.0225)tmp1=82;
        else if(FT1<=59.0926)tmp1=82.1;
        else if(FT1<=59.1626)tmp1=82.2;
        else if(FT1<=59.2328)tmp1=82.3;
        else if(FT1<=59.303)tmp1=82.4;
        else if(FT1<=59.3732)tmp1=82.5;
        else if(FT1<=59.4435)tmp1=82.6;
        else if(FT1<=59.5138)tmp1=82.7;
        else if(FT1<=59.5842)tmp1=82.8;
        else if(FT1<=59.6546)tmp1=82.9;
        else if(FT1<=59.7251)tmp1=83;
    }
    else if(FT1<=60.3613){
        if(FT1==59.7251)tmp1=83;
        else if(FT1<=59.7956)tmp1=83.1;
        else if(FT1<=59.8661)tmp1=83.2;
        else if(FT1<=59.9367)tmp1=83.3;
        else if(FT1<=60.0074)tmp1=83.4;
        else if(FT1<=60.0781)tmp1=83.5;
        else if(FT1<=60.1488)tmp1=83.6;
        else if(FT1<=60.2196)tmp1=83.7;
        else if(FT1<=60.2904)tmp1=83.8;
        else if(FT1<=60.3613)tmp1=83.9;
        else if(FT1<=60.4322)tmp1=84;
    }
    else if(FT1<=61.0726){
        if(FT1==60.4322)tmp1=84;
        else if(FT1<=60.5032)tmp1=84.1;
        else if(FT1<=60.5742)tmp1=84.2;
        else if(FT1<=60.6453)tmp1=84.3;
        else if(FT1<=60.7164)tmp1=84.4;
        else if(FT1<=60.7875)tmp1=84.5;
        else if(FT1<=60.8587)tmp1=84.6;
        else if(FT1<=60.93)tmp1=84.7;
        else if(FT1<=61.0012)tmp1=84.8;
        else if(FT1<=61.0726)tmp1=84.9;
        else if(FT1<=61.144)tmp1=85;
    }
    else if(FT1<=61.7884){
        if(FT1==61.144)tmp1=85;
        else if(FT1<=61.2154)tmp1=85.1;
        else if(FT1<=61.2869)tmp1=85.2;
        else if(FT1<=61.3584)tmp1=85.3;
        else if(FT1<=61.4299)tmp1=85.4;
        else if(FT1<=61.5015)tmp1=85.5;
        else if(FT1<=61.5732)tmp1=85.6;
        else if(FT1<=61.6449)tmp1=85.7;
        else if(FT1<=61.7166)tmp1=85.8;
        else if(FT1<=61.7884)tmp1=85.9;
        else if(FT1<=61.8603)tmp1=86;
    }
    else if(FT1<=62.5089){
        if(FT1==61.8603)tmp1=86;
        else if(FT1<=61.9322)tmp1=86.1;
        else if(FT1<=62.0041)tmp1=86.2;
        else if(FT1<=62.0761)tmp1=86.3;
        else if(FT1<=62.1481)tmp1=86.4;
        else if(FT1<=62.2201)tmp1=86.5;
        else if(FT1<=62.2923)tmp1=86.6;
        else if(FT1<=62.3644)tmp1=86.7;
        else if(FT1<=62.4366)tmp1=86.8;
        else if(FT1<=62.5089)tmp1=86.9;
        else if(FT1<=62.5812)tmp1=87;
    }
    else if(FT1<=63.2339){
        if(FT1==62.5812)tmp1=87;
        else if(FT1<=62.6535)tmp1=87.1;
        else if(FT1<=62.7259)tmp1=87.2;
        else if(FT1<=62.7983)tmp1=87.3;
        else if(FT1<=62.8708)tmp1=87.4;
        else if(FT1<=62.9433)tmp1=87.5;
        else if(FT1<=63.0159)tmp1=87.6;
        else if(FT1<=63.0885)tmp1=87.7;
        else if(FT1<=63.1612)tmp1=87.8;
        else if(FT1<=63.2339)tmp1=87.9;
        else if(FT1<=63.3066)tmp1=88;
    }
    else if(FT1<=63.9634){
        if(FT1==63.3066)tmp1=88;
        else if(FT1<=63.3794)tmp1=88.1;
        else if(FT1<=63.4523)tmp1=88.2;
        else if(FT1<=63.5251)tmp1=88.3;
        else if(FT1<=63.5981)tmp1=88.4;
        else if(FT1<=63.6711)tmp1=88.5;
        else if(FT1<=63.7441)tmp1=88.6;
        else if(FT1<=63.8172)tmp1=88.7;
        else if(FT1<=63.8903)tmp1=88.8;
        else if(FT1<=63.9634)tmp1=88.9;
        else if(FT1<=64.0366)tmp1=89;
    }
    else if(FT1<=64.6976){
        if(FT1==64.0366)tmp1=89;
        else if(FT1<=64.1099)tmp1=89.1;
        else if(FT1<=64.1832)tmp1=89.2;
        else if(FT1<=64.2565)tmp1=89.3;
        else if(FT1<=64.3299)tmp1=89.4;
        else if(FT1<=64.4034)tmp1=89.5;
        else if(FT1<=64.4768)tmp1=89.6;
        else if(FT1<=64.5504)tmp1=89.7;
        else if(FT1<=64.6239)tmp1=89.8;
        else if(FT1<=64.6976)tmp1=89.9;
        else if(FT1<=64.7712)tmp1=90;
    }
    else if(FT1<=65.4363){
        if(FT1==64.7712)tmp1=90;
        else if(FT1<=64.8449)tmp1=90.1;
        else if(FT1<=64.9187)tmp1=90.2;
        else if(FT1<=64.9925)tmp1=90.3;
        else if(FT1<=65.0663)tmp1=90.4;
        else if(FT1<=65.1402)tmp1=90.5;
        else if(FT1<=65.2142)tmp1=90.6;
        else if(FT1<=65.2882)tmp1=90.7;
        else if(FT1<=65.3622)tmp1=90.8;
        else if(FT1<=65.4363)tmp1=90.9;
        else if(FT1<=65.5104)tmp1=91;
    }
    else if(FT1<=66.1795){
        if(FT1==65.5104)tmp1=91;
        else if(FT1<=65.5845)tmp1=91.1;
        else if(FT1<=65.6588)tmp1=91.2;
        else if(FT1<=65.733)tmp1=91.3;
        else if(FT1<=65.8073)tmp1=91.4;
        else if(FT1<=65.8817)tmp1=91.5;
        else if(FT1<=65.9561)tmp1=91.6;
        else if(FT1<=66.0305)tmp1=91.7;
        else if(FT1<=66.105)tmp1=91.8;
        else if(FT1<=66.1795)tmp1=91.9;
        else if(FT1<=66.2541)tmp1=92;
    }
    else if(FT1<=66.9273){
        if(FT1==66.2541)tmp1=92;
        else if(FT1<=66.3287)tmp1=92.1;
        else if(FT1<=66.4034)tmp1=92.2;
        else if(FT1<=66.4781)tmp1=92.3;
        else if(FT1<=66.5528)tmp1=92.4;
        else if(FT1<=66.6276)tmp1=92.5;
        else if(FT1<=66.7025)tmp1=92.6;
        else if(FT1<=66.7774)tmp1=92.7;
        else if(FT1<=66.8523)tmp1=92.8;
        else if(FT1<=66.9273)tmp1=92.9;
        else if(FT1<=67.0023)tmp1=93;
    }
    else if(FT1<=67.6797){
        if(FT1==67.0023)tmp1=93;
        else if(FT1<=67.0774)tmp1=93.1;
        else if(FT1<=67.1525)tmp1=93.2;
        else if(FT1<=67.2277)tmp1=93.3;
        else if(FT1<=67.3029)tmp1=93.4;
        else if(FT1<=67.3782)tmp1=93.5;
        else if(FT1<=67.4535)tmp1=93.6;
        else if(FT1<=67.5288)tmp1=93.7;
        else if(FT1<=67.6042)tmp1=93.8;
        else if(FT1<=67.6797)tmp1=93.9;
        else if(FT1<=67.7551)tmp1=94;
    }
    else if(FT1<=68.4365){
        if(FT1==67.7551)tmp1=94;
        else if(FT1<=67.8307)tmp1=94.1;
        else if(FT1<=67.9062)tmp1=94.2;
        else if(FT1<=67.9819)tmp1=94.3;
        else if(FT1<=68.0575)tmp1=94.4;
        else if(FT1<=68.1332)tmp1=94.5;
        else if(FT1<=68.209)tmp1=94.6;
        else if(FT1<=68.2848)tmp1=94.7;
        else if(FT1<=68.3607)tmp1=94.8;
        else if(FT1<=68.4365)tmp1=94.9;
        else if(FT1<=68.5125)tmp1=95;
    }
    else if(FT1<=69.198){
        if(FT1==68.5125)tmp1=95;
        else if(FT1<=68.5885)tmp1=95.1;
        else if(FT1<=68.6645)tmp1=95.2;
        else if(FT1<=68.7406)tmp1=95.3;
        else if(FT1<=68.8167)tmp1=95.4;
        else if(FT1<=68.8929)tmp1=95.5;
        else if(FT1<=68.9691)tmp1=95.6;
        else if(FT1<=69.0453)tmp1=95.7;
        else if(FT1<=69.1216)tmp1=95.8;
        else if(FT1<=69.198)tmp1=95.9;
        else if(FT1<=69.2744)tmp1=96;
    }
    else if(FT1<=69.9639){
        if(FT1==69.2744)tmp1=96;
        else if(FT1<=69.3508)tmp1=96.1;
        else if(FT1<=69.4273)tmp1=96.2;
        else if(FT1<=69.5038)tmp1=96.3;
        else if(FT1<=69.5804)tmp1=96.4;
        else if(FT1<=69.657)tmp1=96.5;
        else if(FT1<=69.7337)tmp1=96.6;
        else if(FT1<=69.8104)tmp1=96.7;
        else if(FT1<=69.8871)tmp1=96.8;
        else if(FT1<=69.9639)tmp1=96.9;
        else if(FT1<=70.0408)tmp1=97;
    }
    else if(FT1<=70.7344){
        if(FT1==70.0408)tmp1=97;
        else if(FT1<=70.1177)tmp1=97.1;
        else if(FT1<=70.1946)tmp1=97.2;
        else if(FT1<=70.2716)tmp1=97.3;
        else if(FT1<=70.3486)tmp1=97.4;
        else if(FT1<=70.4257)tmp1=97.5;
        else if(FT1<=70.5028)tmp1=97.6;
        else if(FT1<=70.58)tmp1=97.7;
        else if(FT1<=70.6572)tmp1=97.8;
        else if(FT1<=70.7344)tmp1=97.9;
        else if(FT1<=70.8117)tmp1=98;
    }
    else if(FT1<=71.5095){
        if(FT1==70.8117)tmp1=98;
        else if(FT1<=70.8891)tmp1=98.1;
        else if(FT1<=70.9665)tmp1=98.2;
        else if(FT1<=71.0439)tmp1=98.3;
        else if(FT1<=71.1214)tmp1=98.4;
        else if(FT1<=71.1989)tmp1=98.5;
        else if(FT1<=71.2765)tmp1=98.6;
        else if(FT1<=71.3541)tmp1=98.7;
        else if(FT1<=71.4318)tmp1=98.8;
        else if(FT1<=71.5095)tmp1=98.9;
        else if(FT1<=71.5872)tmp1=99;
    }
    else if(FT1<=72.289){
        if(FT1==71.5872)tmp1=99;
        else if(FT1<=71.665)tmp1=99.1;
        else if(FT1<=71.7429)tmp1=99.2;
        else if(FT1<=71.8207)tmp1=99.3;
        else if(FT1<=71.8987)tmp1=99.4;
        else if(FT1<=71.9766)tmp1=99.5;
        else if(FT1<=72.0547)tmp1=99.6;
        else if(FT1<=72.1327)tmp1=99.7;
        else if(FT1<=72.2108)tmp1=99.8;
        else if(FT1<=72.289)tmp1=99.9;
        else if(FT1<=72.3672)tmp1=100;
    }
    else if(FT1<=73.0731){
        if(FT1==72.3672)tmp1=100;
        else if(FT1<=72.4455)tmp1=100.1;
        else if(FT1<=72.5238)tmp1=100.2;
        else if(FT1<=72.6021)tmp1=100.3;
        else if(FT1<=72.6805)tmp1=100.4;
        else if(FT1<=72.7589)tmp1=100.5;
        else if(FT1<=72.8374)tmp1=100.6;
        else if(FT1<=72.9159)tmp1=100.7;
        else if(FT1<=72.9945)tmp1=100.8;
        else if(FT1<=73.0731)tmp1=100.9;
        else if(FT1<=73.1517)tmp1=101;
    }
    else if(FT1<=73.8616){
        if(FT1==73.1517)tmp1=101;
        else if(FT1<=73.2304)tmp1=101.1;
        else if(FT1<=73.3092)tmp1=101.2;
        else if(FT1<=73.388)tmp1=101.3;
        else if(FT1<=73.4668)tmp1=101.4;
        else if(FT1<=73.5457)tmp1=101.5;
        else if(FT1<=73.6246)tmp1=101.6;
        else if(FT1<=73.7036)tmp1=101.7;
        else if(FT1<=73.7826)tmp1=101.8;
        else if(FT1<=73.8616)tmp1=101.9;
        else if(FT1<=73.9407)tmp1=102;
    }
    else if(FT1<=74.6547){
        if(FT1==73.9407)tmp1=102;
        else if(FT1<=74.0199)tmp1=102.1;
        else if(FT1<=74.0991)tmp1=102.2;
        else if(FT1<=74.1783)tmp1=102.3;
        else if(FT1<=74.2576)tmp1=102.4;
        else if(FT1<=74.3369)tmp1=102.5;
        else if(FT1<=74.4163)tmp1=102.6;
        else if(FT1<=74.4957)tmp1=102.7;
        else if(FT1<=74.5752)tmp1=102.8;
        else if(FT1<=74.6547)tmp1=102.9;
        else if(FT1<=74.7343)tmp1=103;
    }
    else if(FT1<=75.4523){
        if(FT1==74.7343)tmp1=103;
        else if(FT1<=74.8139)tmp1=103.1;
        else if(FT1<=74.8935)tmp1=103.2;
        else if(FT1<=74.9732)tmp1=103.3;
        else if(FT1<=75.0529)tmp1=103.4;
        else if(FT1<=75.1327)tmp1=103.5;
        else if(FT1<=75.2125)tmp1=103.6;
        else if(FT1<=75.2924)tmp1=103.7;
        else if(FT1<=75.3723)tmp1=103.8;
        else if(FT1<=75.4523)tmp1=103.9;
        else if(FT1<=75.5323)tmp1=104;
    }
    else if(FT1<=76.2543){
        if(FT1==75.5323)tmp1=104;
        else if(FT1<=75.6123)tmp1=104.1;
        else if(FT1<=75.6924)tmp1=104.2;
        else if(FT1<=75.7726)tmp1=104.3;
        else if(FT1<=75.8527)tmp1=104.4;
        else if(FT1<=75.933)tmp1=104.5;
        else if(FT1<=76.0132)tmp1=104.6;
        else if(FT1<=76.0936)tmp1=104.7;
        else if(FT1<=76.1739)tmp1=104.8;
        else if(FT1<=76.2543)tmp1=104.9;
        else if(FT1<=76.3348)tmp1=105;
    }
    else if(FT1<=77.0609){
        if(FT1==76.3348)tmp1=105;
        else if(FT1<=76.4153)tmp1=105.1;
        else if(FT1<=76.4958)tmp1=105.2;
        else if(FT1<=76.5764)tmp1=105.3;
        else if(FT1<=76.6571)tmp1=105.4;
        else if(FT1<=76.7377)tmp1=105.5;
        else if(FT1<=76.8185)tmp1=105.6;
        else if(FT1<=76.8992)tmp1=105.7;
        else if(FT1<=76.98)tmp1=105.8;
        else if(FT1<=77.0609)tmp1=105.9;
        else if(FT1<=77.1418)tmp1=106;
    }
    else if(FT1<=77.8719){
        if(FT1==77.1418)tmp1=106;
        else if(FT1<=77.2227)tmp1=106.1;
        else if(FT1<=77.3037)tmp1=106.2;
        else if(FT1<=77.3848)tmp1=106.3;
        else if(FT1<=77.4659)tmp1=106.4;
        else if(FT1<=77.547)tmp1=106.5;
        else if(FT1<=77.6282)tmp1=106.6;
        else if(FT1<=77.7094)tmp1=106.7;
        else if(FT1<=77.7906)tmp1=106.8;
        else if(FT1<=77.8719)tmp1=106.9;
        else if(FT1<=77.9533)tmp1=107;
    }
    else if(FT1<=78.6874){
        if(FT1==77.9533)tmp1=107;
        else if(FT1<=78.0347)tmp1=107.1;
        else if(FT1<=78.1161)tmp1=107.2;
        else if(FT1<=78.1976)tmp1=107.3;
        else if(FT1<=78.2791)tmp1=107.4;
        else if(FT1<=78.3607)tmp1=107.5;
        else if(FT1<=78.4423)tmp1=107.6;
        else if(FT1<=78.524)tmp1=107.7;
        else if(FT1<=78.6057)tmp1=107.8;
        else if(FT1<=78.6874)tmp1=107.9;
        else if(FT1<=78.7692)tmp1=108;
    }
    else if(FT1<=79.5074){
        if(FT1==78.7692)tmp1=108;
        else if(FT1<=78.8511)tmp1=108.1;
        else if(FT1<=78.933)tmp1=108.2;
        else if(FT1<=79.0149)tmp1=108.3;
        else if(FT1<=79.0969)tmp1=108.4;
        else if(FT1<=79.1789)tmp1=108.5;
        else if(FT1<=79.261)tmp1=108.6;
        else if(FT1<=79.3431)tmp1=108.7;
        else if(FT1<=79.4252)tmp1=108.8;
        else if(FT1<=79.5074)tmp1=108.9;
        else if(FT1<=79.5897)tmp1=109;
    }
    else if(FT1<=80.3319){
        if(FT1==79.5897)tmp1=109;
        else if(FT1<=79.672)tmp1=109.1;
        else if(FT1<=79.7543)tmp1=109.2;
        else if(FT1<=79.8367)tmp1=109.3;
        else if(FT1<=79.9191)tmp1=109.4;
        else if(FT1<=80.0016)tmp1=109.5;
        else if(FT1<=80.0841)tmp1=109.6;
        else if(FT1<=80.1666)tmp1=109.7;
        else if(FT1<=80.2492)tmp1=109.8;
        else if(FT1<=80.3319)tmp1=109.9;
        else if(FT1<=80.4146)tmp1=110;
    }
    else if(FT1<=81.1608){
        if(FT1==80.4146)tmp1=110;
        else if(FT1<=80.4973)tmp1=110.1;
        else if(FT1<=80.5801)tmp1=110.2;
        else if(FT1<=80.6629)tmp1=110.3;
        else if(FT1<=80.7458)tmp1=110.4;
        else if(FT1<=80.8287)tmp1=110.5;
        else if(FT1<=80.9117)tmp1=110.6;
        else if(FT1<=80.9947)tmp1=110.7;
        else if(FT1<=81.0777)tmp1=110.8;
        else if(FT1<=81.1608)tmp1=110.9;
        else if(FT1<=81.2439)tmp1=111;
    }
    else if(FT1<=81.9942){
        if(FT1==81.2439)tmp1=111;
        else if(FT1<=81.3271)tmp1=111.1;
        else if(FT1<=81.4103)tmp1=111.2;
        else if(FT1<=81.4936)tmp1=111.3;
        else if(FT1<=81.5769)tmp1=111.4;
        else if(FT1<=81.6603)tmp1=111.5;
        else if(FT1<=81.7437)tmp1=111.6;
        else if(FT1<=81.8271)tmp1=111.7;
        else if(FT1<=81.9106)tmp1=111.8;
        else if(FT1<=81.9942)tmp1=111.9;
        else if(FT1<=82.0777)tmp1=112;
    }
    else if(FT1<=82.832){
        if(FT1==82.0777)tmp1=112;
        else if(FT1<=82.1614)tmp1=112.1;
        else if(FT1<=82.245)tmp1=112.2;
        else if(FT1<=82.3288)tmp1=112.3;
        else if(FT1<=82.4125)tmp1=112.4;
        else if(FT1<=82.4963)tmp1=112.5;
        else if(FT1<=82.5802)tmp1=112.6;
        else if(FT1<=82.6641)tmp1=112.7;
        else if(FT1<=82.748)tmp1=112.8;
        else if(FT1<=82.832)tmp1=112.9;
        else if(FT1<=82.916)tmp1=113;
    }
    else if(FT1<=83.6742){
        if(FT1==82.916)tmp1=113;
        else if(FT1<=83.0001)tmp1=113.1;
        else if(FT1<=83.0842)tmp1=113.2;
        else if(FT1<=83.1683)tmp1=113.3;
        else if(FT1<=83.2525)tmp1=113.4;
        else if(FT1<=83.3368)tmp1=113.5;
        else if(FT1<=83.4211)tmp1=113.6;
        else if(FT1<=83.5054)tmp1=113.7;
        else if(FT1<=83.5898)tmp1=113.8;
        else if(FT1<=83.6742)tmp1=113.9;
        else if(FT1<=83.7587)tmp1=114;
    }
    else if(FT1<=84.5209){
        if(FT1==83.7587)tmp1=114;
        else if(FT1<=83.8432)tmp1=114.1;
        else if(FT1<=83.9278)tmp1=114.2;
        else if(FT1<=84.0124)tmp1=114.3;
        else if(FT1<=84.097)tmp1=114.4;
        else if(FT1<=84.1817)tmp1=114.5;
        else if(FT1<=84.2664)tmp1=114.6;
        else if(FT1<=84.3512)tmp1=114.7;
        else if(FT1<=84.436)tmp1=114.8;
        else if(FT1<=84.5209)tmp1=114.9;
        else if(FT1<=84.6058)tmp1=115;
    }
    else if(FT1<=85.372){
        if(FT1==84.6058)tmp1=115;
        else if(FT1<=84.6908)tmp1=115.1;
        else if(FT1<=84.7758)tmp1=115.2;
        else if(FT1<=84.8608)tmp1=115.3;
        else if(FT1<=84.9459)tmp1=115.4;
        else if(FT1<=85.0311)tmp1=115.5;
        else if(FT1<=85.1162)tmp1=115.6;
        else if(FT1<=85.2015)tmp1=115.7;
        else if(FT1<=85.2867)tmp1=115.8;
        else if(FT1<=85.372)tmp1=115.9;
        else if(FT1<=85.4574)tmp1=116;
    }
    else if(FT1<=86.2276){
        if(FT1==85.4574)tmp1=116;
        else if(FT1<=85.5428)tmp1=116.1;
        else if(FT1<=85.6282)tmp1=116.2;
        else if(FT1<=85.7137)tmp1=116.3;
        else if(FT1<=85.7993)tmp1=116.4;
        else if(FT1<=85.8848)tmp1=116.5;
        else if(FT1<=85.9704)tmp1=116.6;
        else if(FT1<=86.0561)tmp1=116.7;
        else if(FT1<=86.1418)tmp1=116.8;
        else if(FT1<=86.2276)tmp1=116.9;
        else if(FT1<=86.3134)tmp1=117;
    }
    else if(FT1<=87.0875){
        if(FT1==86.3134)tmp1=117;
        else if(FT1<=86.3992)tmp1=117.1;
        else if(FT1<=86.4851)tmp1=117.2;
        else if(FT1<=86.571)tmp1=117.3;
        else if(FT1<=86.657)tmp1=117.4;
        else if(FT1<=86.743)tmp1=117.5;
        else if(FT1<=86.8291)tmp1=117.6;
        else if(FT1<=86.9152)tmp1=117.7;
        else if(FT1<=87.0013)tmp1=117.8;
        else if(FT1<=87.0875)tmp1=117.9;
        else if(FT1<=87.1738)tmp1=118;
    }
    else if(FT1<=87.9519){
        if(FT1==87.1738)tmp1=118;
        else if(FT1<=87.26)tmp1=118.1;
        else if(FT1<=87.3464)tmp1=118.2;
        else if(FT1<=87.4327)tmp1=118.3;
        else if(FT1<=87.5192)tmp1=118.4;
        else if(FT1<=87.6056)tmp1=118.5;
        else if(FT1<=87.6921)tmp1=118.6;
        else if(FT1<=87.7787)tmp1=118.7;
        else if(FT1<=87.8653)tmp1=118.8;
        else if(FT1<=87.9519)tmp1=118.9;
        else if(FT1<=88.0386)tmp1=119;
    }
    else if(FT1<=88.8207){
        if(FT1==88.0386)tmp1=119;
        else if(FT1<=88.1253)tmp1=119.1;
        else if(FT1<=88.2121)tmp1=119.2;
        else if(FT1<=88.2989)tmp1=119.3;
        else if(FT1<=88.3857)tmp1=119.4;
        else if(FT1<=88.4726)tmp1=119.5;
        else if(FT1<=88.5596)tmp1=119.6;
        else if(FT1<=88.6465)tmp1=119.7;
        else if(FT1<=88.7336)tmp1=119.8;
        else if(FT1<=88.8207)tmp1=119.9;
        else if(FT1<=88.9078)tmp1=120;
    }
    else if(FT1<=89.6938){
        if(FT1==88.9078)tmp1=120;
        else if(FT1<=88.9949)tmp1=120.1;
        else if(FT1<=89.0821)tmp1=120.2;
        else if(FT1<=89.1694)tmp1=120.3;
        else if(FT1<=89.2567)tmp1=120.4;
        else if(FT1<=89.344)tmp1=120.5;
        else if(FT1<=89.4314)tmp1=120.6;
        else if(FT1<=89.5188)tmp1=120.7;
        else if(FT1<=89.6063)tmp1=120.8;
        else if(FT1<=89.6938)tmp1=120.9;
        else if(FT1<=89.7814)tmp1=121;
    }
    else if(FT1<=90.5714){
        if(FT1==89.7814)tmp1=121;
        else if(FT1<=89.869)tmp1=121.1;
        else if(FT1<=89.9566)tmp1=121.2;
        else if(FT1<=90.0443)tmp1=121.3;
        else if(FT1<=90.132)tmp1=121.4;
        else if(FT1<=90.2198)tmp1=121.5;
        else if(FT1<=90.3076)tmp1=121.6;
        else if(FT1<=90.3955)tmp1=121.7;
        else if(FT1<=90.4834)tmp1=121.8;
        else if(FT1<=90.5714)tmp1=121.9;
        else if(FT1<=90.6593)tmp1=122;
    }
    else if(FT1<=91.4533){
        if(FT1==90.6593)tmp1=122;
        else if(FT1<=90.7474)tmp1=122.1;
        else if(FT1<=90.8355)tmp1=122.2;
        else if(FT1<=90.9236)tmp1=122.3;
        else if(FT1<=91.0118)tmp1=122.4;
        else if(FT1<=91.1)tmp1=122.5;
        else if(FT1<=91.1882)tmp1=122.6;
        else if(FT1<=91.2765)tmp1=122.7;
        else if(FT1<=91.3649)tmp1=122.8;
        else if(FT1<=91.4533)tmp1=122.9;
        else if(FT1<=91.5417)tmp1=123;
    }
    else if(FT1<=92.3396){
        if(FT1==91.5417)tmp1=123;
        else if(FT1<=91.6302)tmp1=123.1;
        else if(FT1<=91.7187)tmp1=123.2;
        else if(FT1<=91.8073)tmp1=123.3;
        else if(FT1<=91.8959)tmp1=123.4;
        else if(FT1<=91.9845)tmp1=123.5;
        else if(FT1<=92.0732)tmp1=123.6;
        else if(FT1<=92.162)tmp1=123.7;
        else if(FT1<=92.2508)tmp1=123.8;
        else if(FT1<=92.3396)tmp1=123.9;
        else if(FT1<=92.4285)tmp1=124;
    }
    else if(FT1<=93.2302){
        if(FT1==92.4285)tmp1=124;
        else if(FT1<=92.5174)tmp1=124.1;
        else if(FT1<=92.6063)tmp1=124.2;
        else if(FT1<=92.6953)tmp1=124.3;
        else if(FT1<=92.7844)tmp1=124.4;
        else if(FT1<=92.8735)tmp1=124.5;
        else if(FT1<=92.9626)tmp1=124.6;
        else if(FT1<=93.0518)tmp1=124.7;
        else if(FT1<=93.141)tmp1=124.8;
        else if(FT1<=93.2302)tmp1=124.9;
        else if(FT1<=93.3196)tmp1=125;
    }
    else if(FT1<=94.1253){
        if(FT1==93.3196)tmp1=125;
        else if(FT1<=93.4089)tmp1=125.1;
        else if(FT1<=93.4983)tmp1=125.2;
        else if(FT1<=93.5877)tmp1=125.3;
        else if(FT1<=93.6772)tmp1=125.4;
        else if(FT1<=93.7667)tmp1=125.5;
        else if(FT1<=93.8563)tmp1=125.6;
        else if(FT1<=93.9459)tmp1=125.7;
        else if(FT1<=94.0356)tmp1=125.8;
        else if(FT1<=94.1253)tmp1=125.9;
        else if(FT1<=94.215)tmp1=126;
    }
    else if(FT1<=95.0247){
        if(FT1==94.215)tmp1=126;
        else if(FT1<=94.3048)tmp1=126.1;
        else if(FT1<=94.3946)tmp1=126.2;
        else if(FT1<=94.4845)tmp1=126.3;
        else if(FT1<=94.5744)tmp1=126.4;
        else if(FT1<=94.6644)tmp1=126.5;
        else if(FT1<=94.7544)tmp1=126.6;
        else if(FT1<=94.8444)tmp1=126.7;
        else if(FT1<=94.9345)tmp1=126.8;
        else if(FT1<=95.0247)tmp1=126.9;
        else if(FT1<=95.1148)tmp1=127;
    }
    else if(FT1<=95.9284){
        if(FT1==95.1148)tmp1=127;
        else if(FT1<=95.2051)tmp1=127.1;
        else if(FT1<=95.2953)tmp1=127.2;
        else if(FT1<=95.3856)tmp1=127.3;
        else if(FT1<=95.476)tmp1=127.4;
        else if(FT1<=95.5664)tmp1=127.5;
        else if(FT1<=95.6568)tmp1=127.6;
        else if(FT1<=95.7473)tmp1=127.7;
        else if(FT1<=95.8378)tmp1=127.8;
        else if(FT1<=95.9284)tmp1=127.9;
        else if(FT1<=96.019)tmp1=128;
    }
    else if(FT1<=96.8364){
        if(FT1==96.019)tmp1=128;
        else if(FT1<=96.1096)tmp1=128.1;
        else if(FT1<=96.2003)tmp1=128.2;
        else if(FT1<=96.2911)tmp1=128.3;
        else if(FT1<=96.3819)tmp1=128.4;
        else if(FT1<=96.4727)tmp1=128.5;
        else if(FT1<=96.5636)tmp1=128.6;
        else if(FT1<=96.6545)tmp1=128.7;
        else if(FT1<=96.7454)tmp1=128.8;
        else if(FT1<=96.8364)tmp1=128.9;
        else if(FT1<=96.9275)tmp1=129;
    }
    else if(FT1<=97.7488){
        if(FT1==96.9275)tmp1=129;
        else if(FT1<=97.0186)tmp1=129.1;
        else if(FT1<=97.1097)tmp1=129.2;
        else if(FT1<=97.2009)tmp1=129.3;
        else if(FT1<=97.2921)tmp1=129.4;
        else if(FT1<=97.3834)tmp1=129.5;
        else if(FT1<=97.4747)tmp1=129.6;
        else if(FT1<=97.566)tmp1=129.7;
        else if(FT1<=97.6574)tmp1=129.8;
        else if(FT1<=97.7488)tmp1=129.9;
        else if(FT1<=97.8403)tmp1=130;
    }
    else if(FT1<=98.6655){
        if(FT1==97.8403)tmp1=130;
        else if(FT1<=97.9318)tmp1=130.1;
        else if(FT1<=98.0234)tmp1=130.2;
        else if(FT1<=98.115)tmp1=130.3;
        else if(FT1<=98.2066)tmp1=130.4;
        else if(FT1<=98.2983)tmp1=130.5;
        else if(FT1<=98.3901)tmp1=130.6;
        else if(FT1<=98.4819)tmp1=130.7;
        else if(FT1<=98.5737)tmp1=130.8;
        else if(FT1<=98.6655)tmp1=130.9;
        else if(FT1<=98.7575)tmp1=131;
    }
    else if(FT1<=99.5866){
        if(FT1==98.7575)tmp1=131;
        else if(FT1<=98.8494)tmp1=131.1;
        else if(FT1<=98.9414)tmp1=131.2;
        else if(FT1<=99.0334)tmp1=131.3;
        else if(FT1<=99.1255)tmp1=131.4;
        else if(FT1<=99.2176)tmp1=131.5;
        else if(FT1<=99.3098)tmp1=131.6;
        else if(FT1<=99.402)tmp1=131.7;
        else if(FT1<=99.4943)tmp1=131.8;
        else if(FT1<=99.5866)tmp1=131.9;
        else if(FT1<=99.6789)tmp1=132;
    }
    else if(FT1<=100.512){
        if(FT1==99.6789)tmp1=132;
        else if(FT1<=99.7713)tmp1=132.1;
        else if(FT1<=99.8637)tmp1=132.2;
        else if(FT1<=99.9562)tmp1=132.3;
        else if(FT1<=100.049)tmp1=132.4;
        else if(FT1<=100.141)tmp1=132.5;
        else if(FT1<=100.234)tmp1=132.6;
        else if(FT1<=100.326)tmp1=132.7;
        else if(FT1<=100.419)tmp1=132.8;
        else if(FT1<=100.512)tmp1=132.9;
        else if(FT1<=100.605)tmp1=133;
    }
    else if(FT1<=101.442){
        if(FT1==100.605)tmp1=133;
        else if(FT1<=100.697)tmp1=133.1;
        else if(FT1<=100.79)tmp1=133.2;
        else if(FT1<=100.883)tmp1=133.3;
        else if(FT1<=100.976)tmp1=133.4;
        else if(FT1<=101.069)tmp1=133.5;
        else if(FT1<=101.162)tmp1=133.6;
        else if(FT1<=101.255)tmp1=133.7;
        else if(FT1<=101.348)tmp1=133.8;
        else if(FT1<=101.442)tmp1=133.9;
        else if(FT1<=101.535)tmp1=134;
    }
    else if(FT1<=102.375){
        if(FT1==101.535)tmp1=134;
        else if(FT1<=101.628)tmp1=134.1;
        else if(FT1<=101.721)tmp1=134.2;
        else if(FT1<=101.815)tmp1=134.3;
        else if(FT1<=101.908)tmp1=134.4;
        else if(FT1<=102.001)tmp1=134.5;
        else if(FT1<=102.095)tmp1=134.6;
        else if(FT1<=102.188)tmp1=134.7;
        else if(FT1<=102.282)tmp1=134.8;
        else if(FT1<=102.375)tmp1=134.9;
        else if(FT1<=102.469)tmp1=135;
    }
    else if(FT1<=103.314){
        if(FT1==102.469)tmp1=135;
        else if(FT1<=102.563)tmp1=135.1;
        else if(FT1<=102.656)tmp1=135.2;
        else if(FT1<=102.75)tmp1=135.3;
        else if(FT1<=102.844)tmp1=135.4;
        else if(FT1<=102.938)tmp1=135.5;
        else if(FT1<=103.032)tmp1=135.6;
        else if(FT1<=103.126)tmp1=135.7;
        else if(FT1<=103.22)tmp1=135.8;
        else if(FT1<=103.314)tmp1=135.9;
        else if(FT1<=103.408)tmp1=136;
    }
    else if(FT1<=104.256){
        if(FT1==103.408)tmp1=136;
        else if(FT1<=103.502)tmp1=136.1;
        else if(FT1<=103.596)tmp1=136.2;
        else if(FT1<=103.69)tmp1=136.3;
        else if(FT1<=103.784)tmp1=136.4;
        else if(FT1<=103.879)tmp1=136.5;
        else if(FT1<=103.973)tmp1=136.6;
        else if(FT1<=104.067)tmp1=136.7;
        else if(FT1<=104.162)tmp1=136.8;
        else if(FT1<=104.256)tmp1=136.9;
        else if(FT1<=104.351)tmp1=137;
    }
    else if(FT1<=105.203){
        if(FT1==104.351)tmp1=137;
        else if(FT1<=104.445)tmp1=137.1;
        else if(FT1<=104.54)tmp1=137.2;
        else if(FT1<=104.634)tmp1=137.3;
        else if(FT1<=104.729)tmp1=137.4;
        else if(FT1<=104.824)tmp1=137.5;
        else if(FT1<=104.918)tmp1=137.6;
        else if(FT1<=105.013)tmp1=137.7;
        else if(FT1<=105.108)tmp1=137.8;
        else if(FT1<=105.203)tmp1=137.9;
        else if(FT1<=105.298)tmp1=138;
    }
    else if(FT1<=106.154){
        if(FT1==105.298)tmp1=138;
        else if(FT1<=105.393)tmp1=138.1;
        else if(FT1<=105.488)tmp1=138.2;
        else if(FT1<=105.583)tmp1=138.3;
        else if(FT1<=105.678)tmp1=138.4;
        else if(FT1<=105.773)tmp1=138.5;
        else if(FT1<=105.868)tmp1=138.6;
        else if(FT1<=105.963)tmp1=138.7;
        else if(FT1<=106.059)tmp1=138.8;
        else if(FT1<=106.154)tmp1=138.9;
        else if(FT1<=106.249)tmp1=139;
    }
    else if(FT1<=107.109){
        if(FT1==106.249)tmp1=139;
        else if(FT1<=106.345)tmp1=139.1;
        else if(FT1<=106.44)tmp1=139.2;
        else if(FT1<=106.536)tmp1=139.3;
        else if(FT1<=106.631)tmp1=139.4;
        else if(FT1<=106.727)tmp1=139.5;
        else if(FT1<=106.822)tmp1=139.6;
        else if(FT1<=106.918)tmp1=139.7;
        else if(FT1<=107.013)tmp1=139.8;
        else if(FT1<=107.109)tmp1=139.9;
        else if(FT1<=107.205)tmp1=140;
    }
    else if(FT1<=108.069){
        if(FT1==107.205)tmp1=140;
        else if(FT1<=107.301)tmp1=140.1;
        else if(FT1<=107.397)tmp1=140.2;
        else if(FT1<=107.492)tmp1=140.3;
        else if(FT1<=107.588)tmp1=140.4;
        else if(FT1<=107.684)tmp1=140.5;
        else if(FT1<=107.78)tmp1=140.6;
        else if(FT1<=107.876)tmp1=140.7;
        else if(FT1<=107.973)tmp1=140.8;
        else if(FT1<=108.069)tmp1=140.9;
        else if(FT1<=108.165)tmp1=141;
    }
    else if(FT1<=109.032){
        if(FT1==108.165)tmp1=141;
        else if(FT1<=108.261)tmp1=141.1;
        else if(FT1<=108.357)tmp1=141.2;
        else if(FT1<=108.454)tmp1=141.3;
        else if(FT1<=108.55)tmp1=141.4;
        else if(FT1<=108.646)tmp1=141.5;
        else if(FT1<=108.743)tmp1=141.6;
        else if(FT1<=108.839)tmp1=141.7;
        else if(FT1<=108.936)tmp1=141.8;
        else if(FT1<=109.032)tmp1=141.9;
        else if(FT1<=109.129)tmp1=142;
    }
    else if(FT1<=110){
        if(FT1==109.129)tmp1=142;
        else if(FT1<=109.226)tmp1=142.1;
        else if(FT1<=109.322)tmp1=142.2;
        else if(FT1<=109.419)tmp1=142.3;
        else if(FT1<=109.516)tmp1=142.4;
        else if(FT1<=109.613)tmp1=142.5;
        else if(FT1<=109.71)tmp1=142.6;
        else if(FT1<=109.806)tmp1=142.7;
        else if(FT1<=109.903)tmp1=142.8;
        else if(FT1<=110)tmp1=142.9;
        else if(FT1<=110.097)tmp1=143;
    }
    else if(FT1<=110.973){
        if(FT1==110.097)tmp1=143;
        else if(FT1<=110.195)tmp1=143.1;
        else if(FT1<=110.292)tmp1=143.2;
        else if(FT1<=110.389)tmp1=143.3;
        else if(FT1<=110.486)tmp1=143.4;
        else if(FT1<=110.583)tmp1=143.5;
        else if(FT1<=110.68)tmp1=143.6;
        else if(FT1<=110.778)tmp1=143.7;
        else if(FT1<=110.875)tmp1=143.8;
        else if(FT1<=110.973)tmp1=143.9;
        else if(FT1<=111.07)tmp1=144;
    }
    else if(FT1<=111.949){
        if(FT1==111.07)tmp1=144;
        else if(FT1<=111.168)tmp1=144.1;
        else if(FT1<=111.265)tmp1=144.2;
        else if(FT1<=111.363)tmp1=144.3;
        else if(FT1<=111.46)tmp1=144.4;
        else if(FT1<=111.558)tmp1=144.5;
        else if(FT1<=111.656)tmp1=144.6;
        else if(FT1<=111.753)tmp1=144.7;
        else if(FT1<=111.851)tmp1=144.8;
        else if(FT1<=111.949)tmp1=144.9;
        else if(FT1<=112.047)tmp1=145;
    }
    else if(FT1<=112.93){
        if(FT1==112.047)tmp1=145;
        else if(FT1<=112.145)tmp1=145.1;
        else if(FT1<=112.243)tmp1=145.2;
        else if(FT1<=112.341)tmp1=145.3;
        else if(FT1<=112.439)tmp1=145.4;
        else if(FT1<=112.537)tmp1=145.5;
        else if(FT1<=112.635)tmp1=145.6;
        else if(FT1<=112.733)tmp1=145.7;
        else if(FT1<=112.831)tmp1=145.8;
        else if(FT1<=112.93)tmp1=145.9;
        else if(FT1<=113.028)tmp1=146;
    }
    else if(FT1<=113.914){
        if(FT1==113.028)tmp1=146;
        else if(FT1<=113.126)tmp1=146.1;
        else if(FT1<=113.225)tmp1=146.2;
        else if(FT1<=113.323)tmp1=146.3;
        else if(FT1<=113.421)tmp1=146.4;
        else if(FT1<=113.52)tmp1=146.5;
        else if(FT1<=113.619)tmp1=146.6;
        else if(FT1<=113.717)tmp1=146.7;
        else if(FT1<=113.816)tmp1=146.8;
        else if(FT1<=113.914)tmp1=146.9;
        else if(FT1<=114.013)tmp1=147;
    }
    else if(FT1<=114.903){
        if(FT1==114.013)tmp1=147;
        else if(FT1<=114.112)tmp1=147.1;
        else if(FT1<=114.211)tmp1=147.2;
        else if(FT1<=114.309)tmp1=147.3;
        else if(FT1<=114.408)tmp1=147.4;
        else if(FT1<=114.507)tmp1=147.5;
        else if(FT1<=114.606)tmp1=147.6;
        else if(FT1<=114.705)tmp1=147.7;
        else if(FT1<=114.804)tmp1=147.8;
        else if(FT1<=114.903)tmp1=147.9;
        else if(FT1<=115.003)tmp1=148;
    }
    else if(FT1<=115.897){
        if(FT1==115.003)tmp1=148;
        else if(FT1<=115.102)tmp1=148.1;
        else if(FT1<=115.201)tmp1=148.2;
        else if(FT1<=115.3)tmp1=148.3;
        else if(FT1<=115.399)tmp1=148.4;
        else if(FT1<=115.499)tmp1=148.5;
        else if(FT1<=115.598)tmp1=148.6;
        else if(FT1<=115.698)tmp1=148.7;
        else if(FT1<=115.797)tmp1=148.8;
        else if(FT1<=115.897)tmp1=148.9;
        else if(FT1<=115.996)tmp1=149;
    }
    else if(FT1<=116.894){
        if(FT1==115.996)tmp1=149;
        else if(FT1<=116.096)tmp1=149.1;
        else if(FT1<=116.195)tmp1=149.2;
        else if(FT1<=116.295)tmp1=149.3;
        else if(FT1<=116.395)tmp1=149.4;
        else if(FT1<=116.494)tmp1=149.5;
        else if(FT1<=116.594)tmp1=149.6;
        else if(FT1<=116.694)tmp1=149.7;
        else if(FT1<=116.794)tmp1=149.8;
        else if(FT1<=116.894)tmp1=149.9;
        else if(FT1<=116.994)tmp1=150;
    }
    else if(FT1<=117.895){
        if(FT1==116.994)tmp1=150;
        else if(FT1<=117.094)tmp1=150.1;
        else if(FT1<=117.194)tmp1=150.2;
        else if(FT1<=117.294)tmp1=150.3;
        else if(FT1<=117.394)tmp1=150.4;
        else if(FT1<=117.494)tmp1=150.5;
        else if(FT1<=117.594)tmp1=150.6;
        else if(FT1<=117.695)tmp1=150.7;
        else if(FT1<=117.795)tmp1=150.8;
        else if(FT1<=117.895)tmp1=150.9;
        else if(FT1<=117.996)tmp1=151;
    }
    else if(FT1<=118.901){
        if(FT1==117.996)tmp1=151;
        else if(FT1<=118.096)tmp1=151.1;
        else if(FT1<=118.197)tmp1=151.2;
        else if(FT1<=118.297)tmp1=151.3;
        else if(FT1<=118.398)tmp1=151.4;
        else if(FT1<=118.498)tmp1=151.5;
        else if(FT1<=118.599)tmp1=151.6;
        else if(FT1<=118.7)tmp1=151.7;
        else if(FT1<=118.8)tmp1=151.8;
        else if(FT1<=118.901)tmp1=151.9;
        else if(FT1<=119.002)tmp1=152;
    }
    else if(FT1<=119.911){
        if(FT1==119.002)tmp1=152;
        else if(FT1<=119.103)tmp1=152.1;
        else if(FT1<=119.203)tmp1=152.2;
        else if(FT1<=119.304)tmp1=152.3;
        else if(FT1<=119.405)tmp1=152.4;
        else if(FT1<=119.506)tmp1=152.5;
        else if(FT1<=119.607)tmp1=152.6;
        else if(FT1<=119.708)tmp1=152.7;
        else if(FT1<=119.81)tmp1=152.8;
        else if(FT1<=119.911)tmp1=152.9;
        else if(FT1<=120.012)tmp1=153;
    }
    else if(FT1<=120.925){
        if(FT1==120.012)tmp1=153;
        else if(FT1<=120.113)tmp1=153.1;
        else if(FT1<=120.215)tmp1=153.2;
        else if(FT1<=120.316)tmp1=153.3;
        else if(FT1<=120.417)tmp1=153.4;
        else if(FT1<=120.519)tmp1=153.5;
        else if(FT1<=120.62)tmp1=153.6;
        else if(FT1<=120.722)tmp1=153.7;
        else if(FT1<=120.823)tmp1=153.8;
        else if(FT1<=120.925)tmp1=153.9;
        else if(FT1<=121.026)tmp1=154;
    }
    else if(FT1<=121.943){
        if(FT1==121.026)tmp1=154;
        else if(FT1<=121.128)tmp1=154.1;
        else if(FT1<=121.23)tmp1=154.2;
        else if(FT1<=121.331)tmp1=154.3;
        else if(FT1<=121.433)tmp1=154.4;
        else if(FT1<=121.535)tmp1=154.5;
        else if(FT1<=121.637)tmp1=154.6;
        else if(FT1<=121.739)tmp1=154.7;
        else if(FT1<=121.841)tmp1=154.8;
        else if(FT1<=121.943)tmp1=154.9;
        else if(FT1<=122.045)tmp1=155;
    }
    else if(FT1<=122.965){
        if(FT1==122.045)tmp1=155;
        else if(FT1<=122.147)tmp1=155.1;
        else if(FT1<=122.249)tmp1=155.2;
        else if(FT1<=122.351)tmp1=155.3;
        else if(FT1<=122.453)tmp1=155.4;
        else if(FT1<=122.556)tmp1=155.5;
        else if(FT1<=122.658)tmp1=155.6;
        else if(FT1<=122.76)tmp1=155.7;
        else if(FT1<=122.862)tmp1=155.8;
        else if(FT1<=122.965)tmp1=155.9;
        else if(FT1<=123.067)tmp1=156;
    }
    else if(FT1<=123.991){
        if(FT1==123.067)tmp1=156;
        else if(FT1<=123.17)tmp1=156.1;
        else if(FT1<=123.272)tmp1=156.2;
        else if(FT1<=123.375)tmp1=156.3;
        else if(FT1<=123.477)tmp1=156.4;
        else if(FT1<=123.58)tmp1=156.5;
        else if(FT1<=123.683)tmp1=156.6;
        else if(FT1<=123.786)tmp1=156.7;
        else if(FT1<=123.888)tmp1=156.8;
        else if(FT1<=123.991)tmp1=156.9;
        else if(FT1<=124.094)tmp1=157;
    }
    else if(FT1<=125.022){
        if(FT1==124.094)tmp1=157;
        else if(FT1<=124.197)tmp1=157.1;
        else if(FT1<=124.3)tmp1=157.2;
        else if(FT1<=124.403)tmp1=157.3;
        else if(FT1<=124.506)tmp1=157.4;
        else if(FT1<=124.609)tmp1=157.5;
        else if(FT1<=124.712)tmp1=157.6;
        else if(FT1<=124.815)tmp1=157.7;
        else if(FT1<=124.918)tmp1=157.8;
        else if(FT1<=125.022)tmp1=157.9;
        else if(FT1<=125.125)tmp1=158;
    }
    else if(FT1<=126.056){
        if(FT1==125.125)tmp1=158;
        else if(FT1<=125.228)tmp1=158.1;
        else if(FT1<=125.331)tmp1=158.2;
        else if(FT1<=125.435)tmp1=158.3;
        else if(FT1<=125.538)tmp1=158.4;
        else if(FT1<=125.642)tmp1=158.5;
        else if(FT1<=125.745)tmp1=158.6;
        else if(FT1<=125.849)tmp1=158.7;
        else if(FT1<=125.952)tmp1=158.8;
        else if(FT1<=126.056)tmp1=158.9;
        else if(FT1<=126.16)tmp1=159;
    }
    else if(FT1<=127.094){
        if(FT1==126.16)tmp1=159;
        else if(FT1<=126.263)tmp1=159.1;
        else if(FT1<=126.367)tmp1=159.2;
        else if(FT1<=126.471)tmp1=159.3;
        else if(FT1<=126.575)tmp1=159.4;
        else if(FT1<=126.679)tmp1=159.5;
        else if(FT1<=126.782)tmp1=159.6;
        else if(FT1<=126.886)tmp1=159.7;
        else if(FT1<=126.99)tmp1=159.8;
        else if(FT1<=127.094)tmp1=159.9;
        else if(FT1<=127.199)tmp1=160;
    }
    else if(FT1<=128.137){
        if(FT1==127.199)tmp1=160;
        else if(FT1<=127.303)tmp1=160.1;
        else if(FT1<=127.407)tmp1=160.2;
        else if(FT1<=127.511)tmp1=160.3;
        else if(FT1<=127.615)tmp1=160.4;
        else if(FT1<=127.72)tmp1=160.5;
        else if(FT1<=127.824)tmp1=160.6;
        else if(FT1<=127.928)tmp1=160.7;
        else if(FT1<=128.033)tmp1=160.8;
        else if(FT1<=128.137)tmp1=160.9;
        else if(FT1<=128.242)tmp1=161;
    }
    else if(FT1<=129.184){
        if(FT1==128.242)tmp1=161;
        else if(FT1<=128.346)tmp1=161.1;
        else if(FT1<=128.451)tmp1=161.2;
        else if(FT1<=128.555)tmp1=161.3;
        else if(FT1<=128.66)tmp1=161.4;
        else if(FT1<=128.765)tmp1=161.5;
        else if(FT1<=128.869)tmp1=161.6;
        else if(FT1<=128.974)tmp1=161.7;
        else if(FT1<=129.079)tmp1=161.8;
        else if(FT1<=129.184)tmp1=161.9;
        else if(FT1<=129.289)tmp1=162;
    }
    else if(FT1<=130.234){
        if(FT1==129.289)tmp1=162;
        else if(FT1<=129.394)tmp1=162.1;
        else if(FT1<=129.499)tmp1=162.2;
        else if(FT1<=129.604)tmp1=162.3;
        else if(FT1<=129.709)tmp1=162.4;
        else if(FT1<=129.814)tmp1=162.5;
        else if(FT1<=129.919)tmp1=162.6;
        else if(FT1<=130.024)tmp1=162.7;
        else if(FT1<=130.129)tmp1=162.8;
        else if(FT1<=130.234)tmp1=162.9;
        else if(FT1<=130.34)tmp1=163;
    }
    else if(FT1<=131.289){
        if(FT1==130.34)tmp1=163;
        else if(FT1<=130.445)tmp1=163.1;
        else if(FT1<=130.55)tmp1=163.2;
        else if(FT1<=130.656)tmp1=163.3;
        else if(FT1<=130.761)tmp1=163.4;
        else if(FT1<=130.867)tmp1=163.5;
        else if(FT1<=130.972)tmp1=163.6;
        else if(FT1<=131.078)tmp1=163.7;
        else if(FT1<=131.184)tmp1=163.8;
        else if(FT1<=131.289)tmp1=163.9;
        else if(FT1<=131.395)tmp1=164;
    }
    else if(FT1<=132.348){
        if(FT1==131.395)tmp1=164;
        else if(FT1<=131.501)tmp1=164.1;
        else if(FT1<=131.606)tmp1=164.2;
        else if(FT1<=131.712)tmp1=164.3;
        else if(FT1<=131.818)tmp1=164.4;
        else if(FT1<=131.924)tmp1=164.5;
        else if(FT1<=132.03)tmp1=164.6;
        else if(FT1<=132.136)tmp1=164.7;
        else if(FT1<=132.242)tmp1=164.8;
        else if(FT1<=132.348)tmp1=164.9;
        else if(FT1<=132.454)tmp1=165;
    }
    else if(FT1<=133.411){
        if(FT1==132.454)tmp1=165;
        else if(FT1<=132.56)tmp1=165.1;
        else if(FT1<=132.666)tmp1=165.2;
        else if(FT1<=132.773)tmp1=165.3;
        else if(FT1<=132.879)tmp1=165.4;
        else if(FT1<=132.985)tmp1=165.5;
        else if(FT1<=133.092)tmp1=165.6;
        else if(FT1<=133.198)tmp1=165.7;
        else if(FT1<=133.304)tmp1=165.8;
        else if(FT1<=133.411)tmp1=165.9;
        else if(FT1<=133.517)tmp1=166;
    }
    else if(FT1<=134.478){
        if(FT1==133.517)tmp1=166;
        else if(FT1<=133.624)tmp1=166.1;
        else if(FT1<=133.731)tmp1=166.2;
        else if(FT1<=133.837)tmp1=166.3;
        else if(FT1<=133.944)tmp1=166.4;
        else if(FT1<=134.05)tmp1=166.5;
        else if(FT1<=134.157)tmp1=166.6;
        else if(FT1<=134.264)tmp1=166.7;
        else if(FT1<=134.371)tmp1=166.8;
        else if(FT1<=134.478)tmp1=166.9;
        else if(FT1<=134.585)tmp1=167;
    }
    else if(FT1<=135.549){
        if(FT1==134.585)tmp1=167;
        else if(FT1<=134.692)tmp1=167.1;
        else if(FT1<=134.799)tmp1=167.2;
        else if(FT1<=134.906)tmp1=167.3;
        else if(FT1<=135.013)tmp1=167.4;
        else if(FT1<=135.12)tmp1=167.5;
        else if(FT1<=135.227)tmp1=167.6;
        else if(FT1<=135.334)tmp1=167.7;
        else if(FT1<=135.441)tmp1=167.8;
        else if(FT1<=135.549)tmp1=167.9;
        else if(FT1<=135.656)tmp1=168;
    }
    else if(FT1<=136.623){
        if(FT1==135.656)tmp1=168;
        else if(FT1<=135.763)tmp1=168.1;
        else if(FT1<=135.871)tmp1=168.2;
        else if(FT1<=135.978)tmp1=168.3;
        else if(FT1<=136.085)tmp1=168.4;
        else if(FT1<=136.193)tmp1=168.5;
        else if(FT1<=136.301)tmp1=168.6;
        else if(FT1<=136.408)tmp1=168.7;
        else if(FT1<=136.516)tmp1=168.8;
        else if(FT1<=136.623)tmp1=168.9;
        else if(FT1<=136.731)tmp1=169;
    }
    else if(FT1<=137.702){
        if(FT1==136.731)tmp1=169;
        else if(FT1<=136.839)tmp1=169.1;
        else if(FT1<=136.947)tmp1=169.2;
        else if(FT1<=137.054)tmp1=169.3;
        else if(FT1<=137.162)tmp1=169.4;
        else if(FT1<=137.27)tmp1=169.5;
        else if(FT1<=137.378)tmp1=169.6;
        else if(FT1<=137.486)tmp1=169.7;
        else if(FT1<=137.594)tmp1=169.8;
        else if(FT1<=137.702)tmp1=169.9;
        else if(FT1<=137.81)tmp1=170;
    }
    else if(FT1<=138.785){
        if(FT1==137.81)tmp1=170;
        else if(FT1<=137.919)tmp1=170.1;
        else if(FT1<=138.027)tmp1=170.2;
        else if(FT1<=138.135)tmp1=170.3;
        else if(FT1<=138.243)tmp1=170.4;
        else if(FT1<=138.351)tmp1=170.5;
        else if(FT1<=138.46)tmp1=170.6;
        else if(FT1<=138.568)tmp1=170.7;
        else if(FT1<=138.677)tmp1=170.8;
        else if(FT1<=138.785)tmp1=170.9;
        else if(FT1<=138.894)tmp1=171;
    }
    else if(FT1<=139.872){
        if(FT1==138.894)tmp1=171;
        else if(FT1<=139.002)tmp1=171.1;
        else if(FT1<=139.111)tmp1=171.2;
        else if(FT1<=139.219)tmp1=171.3;
        else if(FT1<=139.328)tmp1=171.4;
        else if(FT1<=139.437)tmp1=171.5;
        else if(FT1<=139.545)tmp1=171.6;
        else if(FT1<=139.654)tmp1=171.7;
        else if(FT1<=139.763)tmp1=171.8;
        else if(FT1<=139.872)tmp1=171.9;
        else if(FT1<=139.981)tmp1=172;
    }
    else if(FT1<=140.963){
        if(FT1==139.981)tmp1=172;
        else if(FT1<=140.09)tmp1=172.1;
        else if(FT1<=140.199)tmp1=172.2;
        else if(FT1<=140.308)tmp1=172.3;
        else if(FT1<=140.417)tmp1=172.4;
        else if(FT1<=140.526)tmp1=172.5;
        else if(FT1<=140.635)tmp1=172.6;
        else if(FT1<=140.744)tmp1=172.7;
        else if(FT1<=140.853)tmp1=172.8;
        else if(FT1<=140.963)tmp1=172.9;
        else if(FT1<=141.072)tmp1=173;
    }
    else if(FT1<=142.057){
        if(FT1==141.072)tmp1=173;
        else if(FT1<=141.181)tmp1=173.1;
        else if(FT1<=141.291)tmp1=173.2;
        else if(FT1<=141.4)tmp1=173.3;
        else if(FT1<=141.51)tmp1=173.4;
        else if(FT1<=141.619)tmp1=173.5;
        else if(FT1<=141.729)tmp1=173.6;
        else if(FT1<=141.838)tmp1=173.7;
        else if(FT1<=141.948)tmp1=173.8;
        else if(FT1<=142.057)tmp1=173.9;
        else if(FT1<=142.167)tmp1=174;
    }
    else if(FT1<=143.156){
        if(FT1==142.167)tmp1=174;
        else if(FT1<=142.277)tmp1=174.1;
        else if(FT1<=142.387)tmp1=174.2;
        else if(FT1<=142.496)tmp1=174.3;
        else if(FT1<=142.606)tmp1=174.4;
        else if(FT1<=142.716)tmp1=174.5;
        else if(FT1<=142.826)tmp1=174.6;
        else if(FT1<=142.936)tmp1=174.7;
        else if(FT1<=143.046)tmp1=174.8;
        else if(FT1<=143.156)tmp1=174.9;
        else if(FT1<=143.266)tmp1=175;
    }
    else if(FT1<=144.259){
        if(FT1==143.266)tmp1=175;
        else if(FT1<=143.376)tmp1=175.1;
        else if(FT1<=143.486)tmp1=175.2;
        else if(FT1<=143.597)tmp1=175.3;
        else if(FT1<=143.707)tmp1=175.4;
        else if(FT1<=143.817)tmp1=175.5;
        else if(FT1<=143.927)tmp1=175.6;
        else if(FT1<=144.038)tmp1=175.7;
        else if(FT1<=144.148)tmp1=175.8;
        else if(FT1<=144.259)tmp1=175.9;
        else if(FT1<=144.369)tmp1=176;
    }
    else if(FT1<=145.365){
        if(FT1==144.369)tmp1=176;
        else if(FT1<=144.48)tmp1=176.1;
        else if(FT1<=144.59)tmp1=176.2;
        else if(FT1<=144.701)tmp1=176.3;
        else if(FT1<=144.811)tmp1=176.4;
        else if(FT1<=144.922)tmp1=176.5;
        else if(FT1<=145.033)tmp1=176.6;
        else if(FT1<=145.144)tmp1=176.7;
        else if(FT1<=145.254)tmp1=176.8;
        else if(FT1<=145.365)tmp1=176.9;
        else if(FT1<=145.476)tmp1=177;
    }
    else if(FT1<=146.476){
        if(FT1==145.476)tmp1=177;
        else if(FT1<=145.587)tmp1=177.1;
        else if(FT1<=145.698)tmp1=177.2;
        else if(FT1<=145.809)tmp1=177.3;
        else if(FT1<=145.92)tmp1=177.4;
        else if(FT1<=146.031)tmp1=177.5;
        else if(FT1<=146.142)tmp1=177.6;
        else if(FT1<=146.253)tmp1=177.7;
        else if(FT1<=146.364)tmp1=177.8;
        else if(FT1<=146.476)tmp1=177.9;
        else if(FT1<=146.587)tmp1=178;
    }
    else if(FT1<=147.59){
        if(FT1==146.587)tmp1=178;
        else if(FT1<=146.698)tmp1=178.1;
        else if(FT1<=146.809)tmp1=178.2;
        else if(FT1<=146.921)tmp1=178.3;
        else if(FT1<=147.032)tmp1=178.4;
        else if(FT1<=147.144)tmp1=178.5;
        else if(FT1<=147.255)tmp1=178.6;
        else if(FT1<=147.367)tmp1=178.7;
        else if(FT1<=147.478)tmp1=178.8;
        else if(FT1<=147.59)tmp1=178.9;
        else if(FT1<=147.702)tmp1=179;
    }
    else if(FT1<=148.708){
        if(FT1==147.702)tmp1=179;
        else if(FT1<=147.813)tmp1=179.1;
        else if(FT1<=147.925)tmp1=179.2;
        else if(FT1<=148.037)tmp1=179.3;
        else if(FT1<=148.149)tmp1=179.4;
        else if(FT1<=148.26)tmp1=179.5;
        else if(FT1<=148.372)tmp1=179.6;
        else if(FT1<=148.484)tmp1=179.7;
        else if(FT1<=148.596)tmp1=179.8;
        else if(FT1<=148.708)tmp1=179.9;
        else if(FT1<=148.82)tmp1=180;
    }
    else if(FT1<=149.83){
        if(FT1==148.82)tmp1=180;
        else if(FT1<=148.932)tmp1=180.1;
        else if(FT1<=149.044)tmp1=180.2;
        else if(FT1<=149.157)tmp1=180.3;
        else if(FT1<=149.269)tmp1=180.4;
        else if(FT1<=149.381)tmp1=180.5;
        else if(FT1<=149.493)tmp1=180.6;
        else if(FT1<=149.606)tmp1=180.7;
        else if(FT1<=149.718)tmp1=180.8;
        else if(FT1<=149.83)tmp1=180.9;
        else if(FT1<=149.943)tmp1=181;
    }
    else if(FT1<=150.956){
        if(FT1==149.943)tmp1=181;
        else if(FT1<=150.055)tmp1=181.1;
        else if(FT1<=150.168)tmp1=181.2;
        else if(FT1<=150.28)tmp1=181.3;
        else if(FT1<=150.393)tmp1=181.4;
        else if(FT1<=150.505)tmp1=181.5;
        else if(FT1<=150.618)tmp1=181.6;
        else if(FT1<=150.731)tmp1=181.7;
        else if(FT1<=150.843)tmp1=181.8;
        else if(FT1<=150.956)tmp1=181.9;
        else if(FT1<=151.069)tmp1=182;
    }
    else if(FT1<=152.086){
        if(FT1==151.069)tmp1=182;
        else if(FT1<=151.182)tmp1=182.1;
        else if(FT1<=151.295)tmp1=182.2;
        else if(FT1<=151.408)tmp1=182.3;
        else if(FT1<=151.521)tmp1=182.4;
        else if(FT1<=151.634)tmp1=182.5;
        else if(FT1<=151.747)tmp1=182.6;
        else if(FT1<=151.86)tmp1=182.7;
        else if(FT1<=151.973)tmp1=182.8;
        else if(FT1<=152.086)tmp1=182.9;
        else if(FT1<=152.199)tmp1=183;
    }
    else if(FT1<=153.22){
        if(FT1==152.199)tmp1=183;
        else if(FT1<=152.313)tmp1=183.1;
        else if(FT1<=152.426)tmp1=183.2;
        else if(FT1<=152.539)tmp1=183.3;
        else if(FT1<=152.653)tmp1=183.4;
        else if(FT1<=152.766)tmp1=183.5;
        else if(FT1<=152.879)tmp1=183.6;
        else if(FT1<=152.993)tmp1=183.7;
        else if(FT1<=153.106)tmp1=183.8;
        else if(FT1<=153.22)tmp1=183.9;
        else if(FT1<=153.333)tmp1=184;
    }
    else if(FT1<=154.357){
        if(FT1==153.333)tmp1=184;
        else if(FT1<=153.447)tmp1=184.1;
        else if(FT1<=153.561)tmp1=184.2;
        else if(FT1<=153.674)tmp1=184.3;
        else if(FT1<=153.788)tmp1=184.4;
        else if(FT1<=153.902)tmp1=184.5;
        else if(FT1<=154.016)tmp1=184.6;
        else if(FT1<=154.13)tmp1=184.7;
        else if(FT1<=154.244)tmp1=184.8;
        else if(FT1<=154.357)tmp1=184.9;
        else if(FT1<=154.471)tmp1=185;
    }
    else if(FT1<=155.499){
        if(FT1==154.471)tmp1=185;
        else if(FT1<=154.585)tmp1=185.1;
        else if(FT1<=154.699)tmp1=185.2;
        else if(FT1<=154.814)tmp1=185.3;
        else if(FT1<=154.928)tmp1=185.4;
        else if(FT1<=155.042)tmp1=185.5;
        else if(FT1<=155.156)tmp1=185.6;
        else if(FT1<=155.27)tmp1=185.7;
        else if(FT1<=155.385)tmp1=185.8;
        else if(FT1<=155.499)tmp1=185.9;
        else if(FT1<=155.613)tmp1=186;
    }
    else if(FT1<=156.644){
        if(FT1==155.613)tmp1=186;
        else if(FT1<=155.728)tmp1=186.1;
        else if(FT1<=155.842)tmp1=186.2;
        else if(FT1<=155.957)tmp1=186.3;
        else if(FT1<=156.071)tmp1=186.4;
        else if(FT1<=156.186)tmp1=186.5;
        else if(FT1<=156.3)tmp1=186.6;
        else if(FT1<=156.415)tmp1=186.7;
        else if(FT1<=156.529)tmp1=186.8;
        else if(FT1<=156.644)tmp1=186.9;
        else if(FT1<=156.759)tmp1=187;
    }
    else if(FT1<=157.793){
        if(FT1==156.759)tmp1=187;
        else if(FT1<=156.874)tmp1=187.1;
        else if(FT1<=156.988)tmp1=187.2;
        else if(FT1<=157.103)tmp1=187.3;
        else if(FT1<=157.218)tmp1=187.4;
        else if(FT1<=157.333)tmp1=187.5;
        else if(FT1<=157.448)tmp1=187.6;
        else if(FT1<=157.563)tmp1=187.7;
        else if(FT1<=157.678)tmp1=187.8;
        else if(FT1<=157.793)tmp1=187.9;
        else if(FT1<=157.908)tmp1=188;
    }
    else if(FT1<=158.946){
        if(FT1==157.908)tmp1=188;
        else if(FT1<=158.023)tmp1=188.1;
        else if(FT1<=158.139)tmp1=188.2;
        else if(FT1<=158.254)tmp1=188.3;
        else if(FT1<=158.369)tmp1=188.4;
        else if(FT1<=158.484)tmp1=188.5;
        else if(FT1<=158.6)tmp1=188.6;
        else if(FT1<=158.715)tmp1=188.7;
        else if(FT1<=158.831)tmp1=188.8;
        else if(FT1<=158.946)tmp1=188.9;
        else if(FT1<=159.062)tmp1=189;
    }
    else if(FT1<=160.103){
        if(FT1==159.062)tmp1=189;
        else if(FT1<=159.177)tmp1=189.1;
        else if(FT1<=159.293)tmp1=189.2;
        else if(FT1<=159.408)tmp1=189.3;
        else if(FT1<=159.524)tmp1=189.4;
        else if(FT1<=159.64)tmp1=189.5;
        else if(FT1<=159.755)tmp1=189.6;
        else if(FT1<=159.871)tmp1=189.7;
        else if(FT1<=159.987)tmp1=189.8;
        else if(FT1<=160.103)tmp1=189.9;
        else if(FT1<=160.219)tmp1=190;
    }
    else if(FT1<=161.263){
        if(FT1==160.219)tmp1=190;
        else if(FT1<=160.335)tmp1=190.1;
        else if(FT1<=160.451)tmp1=190.2;
        else if(FT1<=160.567)tmp1=190.3;
        else if(FT1<=160.683)tmp1=190.4;
        else if(FT1<=160.799)tmp1=190.5;
        else if(FT1<=160.915)tmp1=190.6;
        else if(FT1<=161.031)tmp1=190.7;
        else if(FT1<=161.147)tmp1=190.8;
        else if(FT1<=161.263)tmp1=190.9;
        else if(FT1<=161.38)tmp1=191;
    }
    else if(FT1<=162.428){
        if(FT1==161.38)tmp1=191;
        else if(FT1<=161.496)tmp1=191.1;
        else if(FT1<=161.612)tmp1=191.2;
        else if(FT1<=161.729)tmp1=191.3;
        else if(FT1<=161.845)tmp1=191.4;
        else if(FT1<=161.961)tmp1=191.5;
        else if(FT1<=162.078)tmp1=191.6;
        else if(FT1<=162.194)tmp1=191.7;
        else if(FT1<=162.311)tmp1=191.8;
        else if(FT1<=162.428)tmp1=191.9;
        else if(FT1<=162.544)tmp1=192;
    }
    else if(FT1<=163.596){
        if(FT1==162.544)tmp1=192;
        else if(FT1<=162.661)tmp1=192.1;
        else if(FT1<=162.778)tmp1=192.2;
        else if(FT1<=162.894)tmp1=192.3;
        else if(FT1<=163.011)tmp1=192.4;
        else if(FT1<=163.128)tmp1=192.5;
        else if(FT1<=163.245)tmp1=192.6;
        else if(FT1<=163.362)tmp1=192.7;
        else if(FT1<=163.479)tmp1=192.8;
        else if(FT1<=163.596)tmp1=192.9;
        else if(FT1<=163.713)tmp1=193;
    }
    else if(FT1<=164.767){
        if(FT1==163.713)tmp1=193;
        else if(FT1<=163.83)tmp1=193.1;
        else if(FT1<=163.947)tmp1=193.2;
        else if(FT1<=164.064)tmp1=193.3;
        else if(FT1<=164.181)tmp1=193.4;
        else if(FT1<=164.298)tmp1=193.5;
        else if(FT1<=164.415)tmp1=193.6;
        else if(FT1<=164.533)tmp1=193.7;
        else if(FT1<=164.65)tmp1=193.8;
        else if(FT1<=164.767)tmp1=193.9;
        else if(FT1<=164.885)tmp1=194;
    }
    else if(FT1<=165.943){
        if(FT1==164.885)tmp1=194;
        else if(FT1<=165.002)tmp1=194.1;
        else if(FT1<=165.12)tmp1=194.2;
        else if(FT1<=165.237)tmp1=194.3;
        else if(FT1<=165.355)tmp1=194.4;
        else if(FT1<=165.472)tmp1=194.5;
        else if(FT1<=165.59)tmp1=194.6;
        else if(FT1<=165.708)tmp1=194.7;
        else if(FT1<=165.825)tmp1=194.8;
        else if(FT1<=165.943)tmp1=194.9;
        else if(FT1<=166.061)tmp1=195;
    }
    else if(FT1<=167.122){
        if(FT1==166.061)tmp1=195;
        else if(FT1<=166.179)tmp1=195.1;
        else if(FT1<=166.296)tmp1=195.2;
        else if(FT1<=166.414)tmp1=195.3;
        else if(FT1<=166.532)tmp1=195.4;
        else if(FT1<=166.65)tmp1=195.5;
        else if(FT1<=166.768)tmp1=195.6;
        else if(FT1<=166.886)tmp1=195.7;
        else if(FT1<=167.004)tmp1=195.8;
        else if(FT1<=167.122)tmp1=195.9;
        else if(FT1<=167.24)tmp1=196;
    }
    else if(FT1<=168.305){
        if(FT1==167.24)tmp1=196;
        else if(FT1<=167.359)tmp1=196.1;
        else if(FT1<=167.477)tmp1=196.2;
        else if(FT1<=167.595)tmp1=196.3;
        else if(FT1<=167.713)tmp1=196.4;
        else if(FT1<=167.832)tmp1=196.5;
        else if(FT1<=167.95)tmp1=196.6;
        else if(FT1<=168.068)tmp1=196.7;
        else if(FT1<=168.187)tmp1=196.8;
        else if(FT1<=168.305)tmp1=196.9;
        else if(FT1<=168.424)tmp1=197;
    }
    else if(FT1<=169.492){
        if(FT1==168.424)tmp1=197;
        else if(FT1<=168.542)tmp1=197.1;
        else if(FT1<=168.661)tmp1=197.2;
        else if(FT1<=168.78)tmp1=197.3;
        else if(FT1<=168.898)tmp1=197.4;
        else if(FT1<=169.017)tmp1=197.5;
        else if(FT1<=169.136)tmp1=197.6;
        else if(FT1<=169.254)tmp1=197.7;
        else if(FT1<=169.373)tmp1=197.8;
        else if(FT1<=169.492)tmp1=197.9;
        else if(FT1<=169.611)tmp1=198;
    }
    else if(FT1<=170.683){
        if(FT1==169.611)tmp1=198;
        else if(FT1<=169.73)tmp1=198.1;
        else if(FT1<=169.849)tmp1=198.2;
        else if(FT1<=169.968)tmp1=198.3;
        else if(FT1<=170.087)tmp1=198.4;
        else if(FT1<=170.206)tmp1=198.5;
        else if(FT1<=170.325)tmp1=198.6;
        else if(FT1<=170.444)tmp1=198.7;
        else if(FT1<=170.563)tmp1=198.8;
        else if(FT1<=170.683)tmp1=198.9;
        else if(FT1<=170.802)tmp1=199;
    }
    else if(FT1<=171.877){
        if(FT1==170.802)tmp1=199;
        else if(FT1<=170.921)tmp1=199.1;
        else if(FT1<=171.04)tmp1=199.2;
        else if(FT1<=171.16)tmp1=199.3;
        else if(FT1<=171.279)tmp1=199.4;
        else if(FT1<=171.399)tmp1=199.5;
        else if(FT1<=171.518)tmp1=199.6;
        else if(FT1<=171.638)tmp1=199.7;
        else if(FT1<=171.757)tmp1=199.8;
        else if(FT1<=171.877)tmp1=199.9;
        else if(FT1<=171.996)tmp1=200;
    }

    return tmp1;
}


