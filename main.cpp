#include "mainwindow.h"
#include <QApplication>
#include "rendering.h"
#include <stdio.h>
#include "showimage.h"
#include "popwindow.h"
#include "tiling.h"
#include "controlpanel.h"
#include "database.h"
#include "targetmodel.h"
#include "loaddatalist.h"
#include "vtkmodel.h"
#include "showdbinfo.h"
#include "calibration.h"
#include "calibrationgraph.h"
#include "controlgraphpanel.h"
#include "showfitsinfo.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    PopWindow p;
    Tiling tt;
    Controlpanel c;
    Database d;
    TargetModel t;
    LoadDataList ls;
    VtkModel v;
    ShowDBInfo i;
    Calibration cali;
    CalibrationGraph cg;
    ControlGraphPanel cgp;
    ShowFITSInfo sfi;


    //メインウィンドウ挙動(消去予定)
    //QObject::connect(&w, SIGNAL(exportDataSignal(QString)), &p, SLOT(getFileName(QString)));
    QObject::connect(&w, SIGNAL(exportDataSignal(QString)), &tt, SLOT(tilingWindow(QString)));
    QObject::connect(&w, SIGNAL(tilingWindowSignal()), &tt, SLOT(show()));

  //  QObject::connect(&cgp, SIGNAL(FITSinfoSignal1(QString*)),&sfi, SLOT(getFITSInfo(QString*)));

    //control panel挙動
    QObject::connect(&c, SIGNAL(showImageSignal()), &ls, SLOT(show()));
    QObject::connect(&c, SIGNAL(showDatabaseSignal()), &d, SLOT(show()));
    QObject::connect(&c, SIGNAL(showTargetModelSignal()), &t, SLOT(show()));
    QObject::connect(&c, SIGNAL(quitSystemSignal()), &a, SLOT(quit()));
    QObject::connect(&c, SIGNAL(showVtkSignal()), &v, SLOT(show()));
    QObject::connect(&c, SIGNAL(showCaliSignal()), &cali, SLOT(show()));
//    QObject::connect(&c, SIGNAL(showControlGraphPanel()), &cgp, SLOT(popControlGraphPanel()));

    QObject::connect(&c, SIGNAL(showControlGraphPanel()), &cgp, SLOT(show()));


    //list挙動
    QObject::connect(&ls, SIGNAL(loadDataSignal(QString,int)), &p, SLOT(getFileName(QString,int)));
    QObject::connect(&ls, SIGNAL(SelectDataSignal(QString,int)), &p, SLOT(getFileName(QString,int)));
    QObject::connect(&ls, SIGNAL(tilingWindowSignal()), &p, SLOT(tilingWindow()));
    QObject::connect(&ls, SIGNAL(closeWindowSignal()), &p, SLOT(closeAll()));
    QObject::connect(&ls, SIGNAL(changeParameterSignal(double,double,int)), &p, SLOT(changeParameter(double,double,int)));
    QObject::connect(&ls, SIGNAL(substractSignal()),&p, SLOT(substract()));


   // QObject::connect(&ls, SIGNAL(FITSinfoSignal(QString*)), &sfi, SLOT(getFITSInfo(QString*)));
    //QObject::connect(&ls, SIGNAL(FITSinfoSignal(QString*)), &sfi, SLOT(show()));

    //database挙動
    QObject::connect(&d, SIGNAL(infoSignal(QString*)), &i, SLOT(getInfo(QString*)));
    QObject::connect(&d, SIGNAL(getFilePathSignal(QString)), &cali, SLOT(getDataPath(QString)));
    QObject::connect(&d, SIGNAL(getFilePathSignal(QString)), &cg, SLOT(getDataPath(QString)));
    QObject::connect(&d, SIGNAL(getFilePathSignal(QString)), &cgp, SLOT(getDataPath(QString)));

    //calibration挙動
    QObject::connect(&cali, SIGNAL(showCalibrationGraphSignal(QVector< QVector<QString> >, QString, QString, QString, int,bool)), &cg, SLOT(popCalibrationGraph(QVector< QVector<QString> >, QString, QString, QString, int,bool)));


    //ControllGraphPanel挙動
    //QObject::connect(&cgp, SIGNAL(showSelectGrphSignal(int,int)), &cg, SLOT(showSelectGraph(int,int)));
    //QObject::connect(&cgp, SIGNAL(showCalibrationPanelSignal()), &cali, SLOT(showCalibrationPanel()));
    //QObject::connect(&cgp, SIGNAL(closeSelectGrphSignal(int,int)), &cg, SLOT(closeSelectGraph(int,int)));
    //QObject::connect(&cg, SIGNAL(removeClalibrationGraphSignal(int,int)), &cgp, SLOT(removePosition(int,int)));
    QObject::connect(&cgp, SIGNAL( changeY(QString) ), &cali, SLOT( setY(QString)) );
    QObject::connect(&cgp, SIGNAL( changeX(QString) ), &cali, SLOT( setX(QString)) );
    //w.show();
    c.show();
    return a.exec();

}
