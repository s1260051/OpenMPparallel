#include "vtkmodel.h"
#include "ui_vtkmodel.h"
#include <QDir>
VtkModel::VtkModel(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VtkModel)
{
    ui->setupUi(this);
    this->setWindowTitle("Thermal model");

    //表示パラメータが変更された時の挙動
    QObject::connect(ui->vtkDataSelect,SIGNAL(currentIndexChanged(int)),ui->vtkRender,SLOT(changeData(int)));
    //色が変更された時の挙動
    QObject::connect(ui->colorSelect,SIGNAL(currentIndexChanged(int)),ui->vtkRender,SLOT(setColor(int)));
    //load vtk from directory buttonの挙動
    QObject::connect(ui->loadVtkButton,SIGNAL(clicked()),this,SLOT(loadVtkData()));
    //directoryパス受け渡しよう関数
    QObject::connect(this,SIGNAL(dirPathSignal(QString)),ui->vtkRender,SLOT(getDirPath(QString)));
    //vtklist クリック挙動
    QObject::connect(ui->vtkList,SIGNAL(currentTextChanged(QString)),ui->vtkRender,SLOT(loadVtkFile(QString)));
    //変更した最大値最小値を渡す
    QObject::connect(this,SIGNAL(changeRangeVtkSignal(double,double)),ui->vtkRender,SLOT(setRangeVtk(double,double)));
}

VtkModel::~VtkModel()
{
    delete ui;
}



void VtkModel::loadVtkData()
{

    //file dialog を使用してディレクトリパス取得
    QDir vtkSrc = QFileDialog::getExistingDirectory(this,"Open Directory",QDir::homePath());
    //読み込んだら一度リストをクリア
    ui->vtkList->clear();

    //printf("%s \n",&vtkSrc.path().toStdString()[0]);

    //ディレクトリが存在したらディレクトリ内のファイル名をリストに追加
     if(vtkSrc.exists()){
          QStringList filelist = vtkSrc.entryList();
          ui->vtkList->addItems(filelist);
     }

     //リストに追加した要素には .(カレントディレクトリ)や..(親ディレクトリ)等が含まれているので除去
     ui->vtkList->takeItem(0);ui->vtkList->takeItem(0);ui->vtkList->takeItem(0);

     //パスを渡す
     emit dirPathSignal(vtkSrc.path());

}

//Apply button挙動
void VtkModel::on_applyButton_clicked()
{
    if(ui->setMin->text()!=NULL && ui->setMax->text()!=NULL)
      emit changeRangeVtkSignal(ui->setMin->text().toDouble(), ui->setMax->text().toDouble());
}
