#ifndef CONTROLGRAPHPANEL_H
#define CONTROLGRAPHPANEL_H

#include <QDialog>
#include <QFileDialog>
#include <QProgressDialog>
#include <QtSql>
#include "database.h"
#include <FITS.h>


namespace Ui {
class ControlGraphPanel;
}


//direct conversion用構造体
struct imageData {
    QVector<double> image;
    double bol_t;
    double pkg_t;
    double case_t;
    double shut_t;
    double lens_t;
    double target_t;
    double distance;
};

struct interpolateFunction {
    double DN;
    double targetT;
    double distAve;
};


class ControlGraphPanel : public QDialog
{
    Q_OBJECT

public:
    explicit ControlGraphPanel(QWidget *parent = 0);
    ~ControlGraphPanel();
    ControlGraphPanel *t[50];

    int n;

    void DarkImage();

    void on_calibrationButton_clicked();
    void on_BlackbodycalibrationAllPixelButton_clicked();
    void on_BlackbodycalibrationAllPixelButton_repeat_clicked();
    void on_calibrationtoRadianceButton_clicked();
    void on_directConversionButton_clicked();

private:
    Ui::ControlGraphPanel *ui;
    QList<QString> position;
    int i,j;
    double fitsave=0;

    QString fileName;
    QString subFileName;

    //接続用
    QSqlDatabase db;
    QSqlQuery query;
    QSqlQuery pairQuery;
    QString databPath;



signals:
    void showSelectGrphSignal(int, int);
    void showCalibrationPanelSignal();
    void closeSelectGrphSignal(int, int);
    void changeX(QString);
    void changeY(QString);
    void FITSinfoSignal1(QString*);

    void exportfilename(QString);



public slots:
    void getDataPath(QString);


private slots:
    void popControlGraphPanel();
    void on_loadFileButton_clicked();
    void on_substructButton_clicked();
    void on_substructFITSButton_clicked();
 //   void on_calibrationButton_clicked();
    //void on_confirmation_clicked();
   // void on_BlackbodycalibrationAllPixelButton_clicked();
    //void on_BlackbodycalibrationAllPixelButton_repeat_clicked();
    //void on_calibrationtoRadianceButton_clicked();
    void setX(QString x);
    void setY(QString y);
    //void on_directConversionButton_clicked();
    void on_outputCurrentImageButton_clicked();
    //void FITSinfoSignal(QString*);
    void on_StartConversionButton_clicked();

    void connectDB();

    static bool compDist(const imageData e1, const imageData e2);


};

#endif // CONTROLGRAPHPANEL_H
