#include "mainwindow.h"
#include "rendering.h"
#include <fstream>
#include <iostream>


//#define NUM 49152
#define NUM 196608

int facetN=0;
int model=0;
GLdouble normal[NUM][3];

//light0の証明条件
GLfloat light0pos[] = {0.0,0.0,1.0,1};
GLfloat light0amb[] = {0.1,0.1,0.1,1.0 };
GLfloat light0dif[] = {0.18,0.18,0.18,1.0 };
GLfloat light0spe[] = {0.2,0.2,0.2,1.0 };

Rendering::Rendering(QWidget *parent) :
    QOpenGLWidget(parent)
{
    //モデルのソースパス
    srcPath.cd(QCoreApplication::applicationDirPath());
 //   srcPath.cd("../../modelSrc");

}

Rendering::~Rendering()
{

}

void Rendering::initializeGL()
{
    RotX=0;
    RotY=0;
    num=0;
    //形状モデル読み込み
    loadFile();

    glClearColor(0, 0, 0, 0);
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    glEnable(GL_LIGHTING);    /* ライティング処理のオン */
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light0pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light0amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0dif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0spe);
}

void Rendering::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    glLoadIdentity();
    //視野指定
    glOrtho(-247.0, 247.0, -138.0, 138.0,-700.0, 700.0);

}

void Rendering::paintGL()
{
    float ambient[] = { 0.01, 0.01, 0.01, 1.0};
    float diffuse[] = { 0.014, 0.014, 0.014, 1.0};
    float specular[]= { 0.01, 0.01, 0.01, 1.0};

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glRotated(2.0,RotX,RotY,0.0);
    glMaterialfv(GL_FRONT,GL_AMBIENT,ambient);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,diffuse);
    glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
    glMaterialf(GL_FRONT,GL_SHININESS,20);

    render();

}


//描画関数
void Rendering::render()
{


    glBegin(GL_TRIANGLES);


    //描画
    for(int i=0;i<facetN;i++){
        glNormal3dv(normal[i]);
        for(int j=0;j<3;j++){
          //3点から構成される面を描画
          glVertex3dv(point[facet[i][j]]);
        }
    }
    glEnd();
}

//形状モデル、ポリゴン、読み込み関数

void Rendering::loadFile()
{
    double V;
    QString str;
    QFile filep;
    QFile filef;

    if(model==0){
        //Itokawaモデル読み込み
        filep.setFileName(srcPath.path()+"/ItokawaPoly.txt");
        filef.setFileName(srcPath.path()+"/ItokawaPolyID.txt");
        //表示倍率指定
        V=0.6;
    }
    else if(model==1) {
        //Ryuguモデル読み込み
        filep.setFileName(srcPath.path()+"/RyuguPoly.txt");
        filef.setFileName(srcPath.path()+"/RyuguPolyID.txt");
        //表示倍率指定
        V=200;
    }


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
        //データ確認用 printf("%d\n",point[i][j]);
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

    //法線ベクトル計算
    setNormalVector();

}

//消去予定
double Rendering::colorDefine(double T,int i)
{
   return T/maxT[i];
}

//マウスイベント関数クリック挙動
void Rendering::mousePressEvent(QMouseEvent* event)
{
    if(event->buttons() & Qt::LeftButton){
     lastPos = event->pos();
    }
    //消去予定
    else if(event->buttons() & Qt::RightButton){
        num++;
     if(num==3)num=0;
     update();
    }

}

//マウスイベント関数ドラッグ挙動
void Rendering::mouseMoveEvent(QMouseEvent *event)
{
     double dx = event->x() - lastPos.x();
     double dy = event->y() - lastPos.y();

     if(event->buttons() & Qt::LeftButton){

     if(dx>lastPos.x()){
         RotX=dx;
     }else{RotX=-dx;}

     if(dy>lastPos.y()){
         RotY=dy;
     }else{RotY=-dy;}

     }
//位置記憶
   lastPos = event->pos();
   update();
}

//モデル変更関数
void Rendering::changeModel(int x){
    model =x;
    facetN=0;
    loadFile();
    update();
}

//法線ベクトル定義
void Rendering::setNormalVector()
{
    GLdouble tmp[3][3];

    for(int i=0;i<facetN;i++){
        for(int j=0;j<3;j++){
          tmp[j][0]= point[facet[i][j]][0];
          tmp[j][1]= point[facet[i][j]][1];
          tmp[j][2]= point[facet[i][j]][2];
        }
        calcNormalVector(tmp,i);

    }

}

//法線ベクトル計算
void Rendering::calcNormalVector(GLdouble a[3][3],int n)
{
    GLdouble v1[3], v2[3], cross[3];
        for (int i = 0; i < 3; i++){ v1[i] = a[0][i] - a[1][i]; }
        for (int i = 0; i < 3; i++){ v2[i] = a[2][i] - a[1][i]; }
        for (int i = 0; i < 3; i++){ cross[i] = v2[(i+1)%3] * v1[(i+2)%3] - v2[(i+2)%3] * v1[(i+1)%3]; }
        double length = sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);

        for (int i = 0; i < 3; i++) {
            if(length !=0) normal[n][i] = cross[i] / length;
            else normal[n][i] = 0 ;
        }
}
