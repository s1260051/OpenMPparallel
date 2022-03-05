#include "showdbinfo.h"
#include "ui_showdbinfo.h"

ShowDBInfo::ShowDBInfo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ShowDBInfo)
{
    ui->setupUi(this);

}

ShowDBInfo::~ShowDBInfo()
{
    delete ui;
}

//表示要素受け取り、表示
void ShowDBInfo::getInfo(QString* info)
{

    ui->dbInfoList->clear();
    //要素の数だけ回す
    for(int i=0;i<18;i++)
    ui->dbInfoList->addItem(info[i]);

    //ファイル名表示
    this->setWindowTitle(info[17].mid(17));
    this->show();
    this->raise();
}
