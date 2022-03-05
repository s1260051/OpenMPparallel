#include "controlpanel.h"
#include "ui_controlpanel.h"
#include "database.h"
#include <QFileDialog>

Controlpanel::Controlpanel(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Controlpanel)
{
    ui->setupUi(this);
    this->setWindowTitle("Control");
    //ディスプレイ上部に移動
    this->move(400,30);

}

Controlpanel::~Controlpanel()
{
    delete ui;
}

//3Dモデルwindow呼び出し
void Controlpanel::on_showModelButton_clicked()
{
    emit showTargetModelSignal();
}

//DBwindow呼び出し
void Controlpanel::on_showDBButton_clicked()
{
    emit showDatabaseSignal();
}

//TIRimage window呼び出し
void Controlpanel::on_showImageButton_clicked()
{
    emit showImageSignal();
}

//システム終了
void Controlpanel::on_quitButton_clicked()
{
    emit quitSystemSignal();
}

//Vtk描画 window呼び出し
void Controlpanel::on_showVtkButton_clicked()
{
    emit showVtkSignal();
}

//キャリブレーションパネルの呼び出し
void Controlpanel::on_showCaliButton_clicked()
{
    emit showCaliSignal();
  //  emit showControlGraphPanel();
}

void Controlpanel::on_showConversionButton_clicked(){
    emit showControlGraphPanel();
}
