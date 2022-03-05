#include "vtkrendering.h"
#include <fstream>
#include <iostream>

double tempareture[NUM]={};
double directflux[NUM]={};
double radiationflux[NUM]={};
double viewfactor[NUM]={};

bool judge=true;
double T=0;
int setdata = 0;
int colorset=0;
QString dirPath;

VtkRendering::VtkRendering(QWidget *parent) :
    QOpenGLWidget(parent)
{
    //初期化
    facetN=0;
    for(int i=0;i<4;i++){
        MAX[i]=-10000;
        MIN[i]= 10000;
    }
    makeColorTable();

}

VtkRendering::~VtkRendering()
{
}

void VtkRendering::initializeGL()
{
    RotX=0;
    RotY=0;
    num=0;
    //形状モデル読み込み
    loadFile();

    glClearColor(0, 0, 0, 1);
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

}

void VtkRendering::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    glLoadIdentity();
    //視野指定
    glOrtho(-247.0, 247.0, -138.0, 138.0,-700.0, 700.0);

}

//描画関数
void VtkRendering::paintGL()
{

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);


    colorBar();

    glPushMatrix();
    glRotated(RotX+RotY,RotX,RotY,0);
    glTranslated(-50,0,0);
    render();
    glPopMatrix();
}


//描画関数
void VtkRendering::render()
{

    glBegin(GL_TRIANGLES);
    //描画
    for(int i=0;i<NUM;i++){
        for(int k=0;k<256;k++){
            if(tempareture[i]<colorValue[k]){
                glColor3dv(colorTable[k]);
                for(int j=0;j<3;j++){
                    //3点から構成される面を描画
                    glVertex3dv(point[facet[i][j]]);
                }
                break;
            }
        }
    }
    glEnd();
}

//形状モデル、ポリゴン、読み込み関数
void VtkRendering::loadFile()
{
    double V;
    QString str;
    QFile filep;
    QFile filef;

    //現時点ではイトカワのモデルのみ描画->Vtkファイル内のモデルデータを読むようにする
    QString appPath;
    appPath = QCoreApplication::applicationDirPath();
/*
    filep.setFileName(appPath+"/ItokawaPoly.txt");
    filef.setFileName(appPath+"/ItokawaPolyID.txt");
*/


    filep.setFileName(appPath+"/myRyuguPoly2.txt");
    filef.setFileName(appPath+"/myRyuguPolyID.txt");


    //filep.setFileName("../../../modelSrc/ItokawaPoly.txt");
    //filef.setFileName("../../../modelSrc/ItokawaPolyID.txt");
    // V=0.6;

    V=100;

    if (!filep.open(QIODevice::ReadOnly)||!filef.open(QIODevice::ReadOnly))//読込のみでオープンできたかチェック
    {
        printf("error\n");
        return;
    }

    QTextStream in_p(&filep);
    QTextStream in_f(&filef);


    //point読み込み
    for(int i=0;!in_p.atEnd();i++){
        for(int j=0;j<3;j++){
            in_p >> str;
            //点の座標x,y,z
            point[i][j]=str.toDouble()*V;

            //データ確認用printf("%d\n",point[i][j]);
        }
    }

    //facet読み込み
    for(int i=0;!in_f.atEnd();i++){
        for(int j=0;j<3;j++){
            in_f >> str;
            //facetを構成する点の番号3つ
            facet[i][j]=str.toInt();
            //データ確認用 printf("%d\n",facet[i][j]);
        }
        facetN++;
    }

}

//マウスイベント関数
void VtkRendering::mousePressEvent(QMouseEvent* event)
{
    if(event->buttons() & Qt::LeftButton){
        lastPos = event->pos();
    }else if(event->buttons() & Qt::RightButton){
        if(judge ==true)judge=false;
        else judge = true;
        update();
    }

}

void VtkRendering::mouseMoveEvent(QMouseEvent *event)
{
    double dx = event->x() - lastPos.x();
    double dy = event->y() - lastPos.y();

    if(event->buttons() & Qt::LeftButton){

        if(dx>lastPos.x()){
            RotX+=dx;
        }else{RotX+=-dx;}

        if(dy>lastPos.y()){
            RotY+=dy;
        }else{RotY+=-dy;}

    }

    lastPos = event->pos();
    update();
}

//vtkファイル読み込み
void VtkRendering::loadVtkFile(QString vtkFileName)//vtkファイルそのものを読み込む
{

    //QFile vtkFile("/Users/joker/test/Qt/4dcube_viewer/vtksrc/Itokawa64q_global_I0-R_part1/" + vtkFileName);
    QFile vtkFile(dirPath+"/"+vtkFileName);
    QString str;
    int count[4]={0,0,0,0};
    //初期化
    for(int i=0;i<4;i++){
        MAX[i]=-10000;
        MIN[i]= 10000;
    }

    if (!vtkFile.open(QIODevice::ReadOnly)){
        printf("error\n");
        return;
    }

    QTextStream vtk(&vtkFile);

    //vtkファイル読み込み->現在決め打ちで読み込んでいるため、モデルデータの大きさによって読み込む範囲を指定できるようになると良い
    for(int i=0;!vtk.atEnd();i++){
        //Itokawa
/*
        if(73741<= i && i<73740+NUM){
            vtk >> str;
            if(str.toDouble() > MAX[0]) MAX[0] = str.toDouble();
            if(str.toDouble() < MIN[0]) MIN[0] = str.toDouble();

            tempareture[count[0]] = str.toDouble();
            count[0]++;
        }

        else if(122897<= i && i<122897+NUM){
            vtk >> str;
            if(str.toDouble() > MAX[1]) MAX[1] = str.toDouble();
            if(str.toDouble() < MIN[1]) MIN[1] = str.toDouble();

            directflux[count[1]] = str.toDouble();
            count[1]++;
        }
        else if(172053<= i && i<172053+NUM){
            vtk >> str;
            if(str.toDouble() > MAX[2]) MAX[2] = str.toDouble();
            if(str.toDouble() < MIN[2]) MIN[2] = str.toDouble();

            radiationflux[count[2]] = str.toDouble();
            count[2]++;
        }
        else if(221209<= i && i<221209+NUM){
            vtk >> str;
            if(str.toDouble() > MAX[3]) MAX[3] = str.toDouble();
            if(str.toDouble() < MIN[3]) MIN[3] = str.toDouble();

            viewfactor[count[3]] = str.toDouble();
            count[3]++;
        }

        else str = vtk.readLine(0);
*/

        //Ryugu-------------------------------------------------------------------

        if(296464<= i && i<296464+NUM){
            vtk >> str;
            if(str.toDouble() > MAX[0]) MAX[0] = str.toDouble();
            if(str.toDouble() < MIN[0]) MIN[0] = str.toDouble();

            tempareture[count[0]] = str.toDouble();
            count[0]++;
        }

        else str = vtk.readLine(0);

        //------------------------------------------------------------------------


    }
    vtkFile.close();

    //カラーテーブル設定
    T = MAX[setdata] - MIN[setdata];
    double d=T/256;

    for(int i=0;i<256;i++){
        colorValue[i]= d*i+MIN[setdata];
    }


    update();
}

//カラーテーブル作成 showimageクラスと一緒
void VtkRendering::makeColorTable(){
    int colorArea;
    double count = 1;

    if(colorset==0){
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
    if(colorset==1)
        for(int i=0;i<256;i++){
            colorTable[i][0] = (double)i/255;
            colorTable[i][1] = (double)i/255;
            colorTable[i][2] = (double)i/255;
        }

    if(colorset==2){
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

//カラーバー描画
void VtkRendering::colorBar()
{
    if(judge==true){

        glBegin(GL_POINTS);

        for(int i=0;i<256;i++){
            glColor3dv(colorTable[i]);
            for(int j=0;j<15;j++){
                //glVertex2d(j+395,i);
                glVertex2d(j+160,i-125);
            }
        }
        glEnd();

        //
        QPainter num(this);
        num.setPen(Qt::cyan);
        num.setFont(QFont("Arial", 15));
        num.drawText(427,24,QString::number((int)MAX[setdata]));
        num.drawText(427,85,QString::number((int)(MAX[setdata]-T/4)));
        num.drawText(427,146,QString::number((int)(MAX[setdata]-T/2)));
        num.drawText(427,206,QString::number((int)(MAX[setdata]-T*3/4)));
        num.drawText(427,265,QString::number((int)MIN[setdata]));
        num.end();
    }
}






//範囲変更用関数
void VtkRendering::changeData(int x)
{
    setdata = x;

    //カラーテーブルに反映させる数値指定
    T = MAX[setdata] - MIN[setdata];
    double d=T/256;

    for(int i=0;i<256;i++){
        colorValue[i]= d*i+MIN[setdata];
    }

    this->update();
}

//ディレクトリパス受け取り用関数
void VtkRendering::getDirPath(QString n){
    dirPath = n;
}

//LUT変更用関数
void VtkRendering::setColor(int x)
{
    colorset = x;
    makeColorTable();

    update();
}

//範囲を変えた場合の処理用関数
void VtkRendering::setRangeVtk(double min,double max)
{

    MAX[setdata] = max;
    MIN[setdata] = min;

    T = MAX[setdata] - MIN[setdata];
    double d=T/256;

    for(int i=0;i<256;i++){
        colorValue[i]= d*i+MIN[setdata];
    }

    makeColorTable();
    update();
}
