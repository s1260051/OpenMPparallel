#include "showfitsinfo.h"
#include "ui_showfitsinfo.h"
#include "loaddatalist.h"
#include "controlgraphpanel.h"

ShowFITSInfo::ShowFITSInfo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ShowFITSInfo)
{
    ui->setupUi(this);
    this->move(520,0);
}

ShowFITSInfo::~ShowFITSInfo()
{
    delete ui;
}

//表示要素受け取り、表示

void ShowFITSInfo::getFITSInfo(QString* info)
{

    t[n] = new ShowFITSInfo;
    t[n]->ui->fitsInfoList->clear();
    //要素の数だけ回す
  //  LoadDataList LDT;

    for(int i=0;i<=76;i++)
    t[n]->ui->fitsInfoList->addItem(info[i]);

    //ファイル名表示
    t[n]->setWindowTitle("FITS header");
    t[n]->show();
    t[n]->raise();

    n++;
}
