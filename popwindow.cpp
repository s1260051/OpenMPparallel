#include "popwindow.h"
#include "ui_popwindow.h"
#include "showimage.h"
#include "mainwindow.h"
#include <iostream>

PopWindow::PopWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PopWindow)
{
    ui->setupUi(this);
    n=0;
}

PopWindow::~PopWindow()
{
    delete ui;
}

//ファイル読み込み
void PopWindow::getFileName(QString name,int x){
    //宣言
    t[n] = new PopWindow;
    //ファイル名
    importName[n] = name;
    //ファイル名長さ
    nameLength[n] = x;
    ui->setupUi(t[n]);
    //windowtitle設定
    t[n]->setWindowTitle(importName[n].right(nameLength[n]));
    //ファイル名を描画クラスへ渡す
    ui->anotherView->loadFileName(importName[n]);
    //window表示
    t[n]->show();
    t[n]->activateWindow();
    n++;

}

//Tiling動作関数
void PopWindow::tilingWindow(){
    int j=0,k=0;
    //1列3つずつ表示する
    for(int i = 1; i<n+1;i++,k++){
        if(i%4==0){
            j++;
            k=0;
        }
     //デスクトップ左上から横470,縦330ずらして表示
    t[i-1]->move(0+k*470,0+j*330);
    }
}

//window消去用関数
void PopWindow::closeAll(){
    for(int i = 0; i<n;i++){
        t[i]->destroy();
    }
    //nを初期化
    n=0;
}

//パラメータ変更用関数
void PopWindow::changeParameter(double min, double max, int x)
{
    //windowの位置を記憶しwindow消去
    for(int i = 0; i<n;i++){
        position[i] = t[i]->pos();
        t[i]->destroy();
    }

    //現在作成されているwindowの数だけループ、各設定を変更し再描画
    for(int i = 0; i<n;i++){
        t[i] = new PopWindow;
        ui->setupUi(t[i]);
        t[i]->move(position[i]);
        ui->anotherView->loadFileName(importName[i]);
        ui->anotherView->changeParameter(min,max,x);
        t[i]->setWindowTitle(importName[i].right(nameLength[i]));
        t[i]->update();
        t[i]->show();
    }
}

void PopWindow::substract()
{

        std::cout<<"pop"<<std::endl;
    t[n] = new PopWindow;
    ui->setupUi(t[n]);
    //windowtitle設定
    t[n]->setWindowTitle(importName[n-1].right(nameLength[n-1])+"sub");
    //ファイル名を描画クラスへ渡す
   ui->anotherView->subtractImage(importName[n-1],importName[n-2]);

    //window表示
    t[n]->show();
    t[n]->activateWindow();
    n++;

    std::cout<<"pop"<<std::endl;
}



