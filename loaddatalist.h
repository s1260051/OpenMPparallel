#ifndef LOADDATALIST_H
#define LOADDATALIST_H

#include <QDialog>
#include <QFileDialog>
#include <FITS.h>
#include <iostream>
#include <fstream>
using namespace std;

namespace Ui {
class LoadDataList;
}

class LoadDataList : public QDialog
{
    Q_OBJECT

public:
    explicit LoadDataList(QWidget *parent = 0);
    ~LoadDataList();
   // int keywordcountforshowfitsinfo();

private slots:
    void on_loadedDataList_clicked(const QModelIndex &index);
    void on_tilingButton_clicked();
    void on_closeAllButton_clicked();
    void on_loadFileButton_clicked();

    void on_changeParameterButton_clicked();

    void on_substract_clicked();

private:
    Ui::LoadDataList *ui;

public slots:

signals:
    void SelectDataSignal(QString,int);
    void loadDataSignal(QString,int);
    void tilingWindowSignal();
    void closeWindowSignal();
    void changeParameterSignal(double,double,int);
    void substractSignal();
    void FITSinfoSignal(QString*);
};

#endif // LOADDATALIST_H
