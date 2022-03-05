#ifndef CALIBRATIONGRAPH_H
#define CALIBRATIONGRAPH_H

#include "qcustomplot.h"
#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QtSql>
#include <algorithm>
#include <math.h>
#include <sstream>
#define PI (4.0*atan(1.0))

//thread用global変数
namespace Ui {
class CalibrationGraph;
}

class ForThread1 : public QThread
{
    Q_OBJECT
public:
    void run();
    QString name;

private:
    QSqlQuery query_1;
 void saveFileInRegisterRegression(QString fileName, QString folder);



public slots:
    // 作業用スロット
};

class ForThread2 : public QThread
{
    Q_OBJECT
public:
    void run();
    QString name;
private:
    QSqlQuery query_2;
 void saveFileInRegisterRegression(QString fileName, QString folder);

public slots:
    // 作業用スロット
};

class ForThread3 : public QThread
{
    Q_OBJECT
public:
    void run();
    QString name;

private:
    QSqlQuery query_3;
 void saveFileInRegisterRegression(QString fileName, QString folder);


public slots:
    // 作業用スロット
};

class ForThread4 : public QThread
{
    Q_OBJECT
public:
    void run();
    QString name;
private:
    QSqlQuery query_4;
 void saveFileInRegisterRegression(QString fileName, QString folder);

public slots:
    // 作業用スロット
};

class ForThread5 : public QThread
{
    Q_OBJECT
public:
    void run();
    QString name;
private:
    QSqlQuery query_5;
 void saveFileInRegisterRegression(QString fileName, QString folder);

public slots:
    // 作業用スロット
};

class ForThread6 : public QThread
{
    Q_OBJECT
public:
    void run();
    QString name;
private:
    QSqlQuery query_6;
 void saveFileInRegisterRegression(QString fileName, QString folder);

public slots:
    // 作業用スロット
};



double planck4(double);


class CalibrationGraph : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrationGraph(QWidget *parent = 0);
    ~CalibrationGraph();
    CalibrationGraph *cg[10000];
    QVector<QVector<QString> > pixelList; //キャリブレーションパネルからもらってきたピクセルリスト本体
    QVector<QVector<QString> > pixelList2; //open、close、diff等で絞られたリストを格納すリスト
    QVector<QVector<QString> > diff; //diff関連をリプロットする際に時間がかかるため、対策としてdiffの情報を保持する用
    QVector<QVector<QString> > replot; //リプロット関連とcsvファイル出力の際に使用するピクセルリスト
   // QString residual[248][328]; //画像毎登録の際に残差をテキストファイルを作成する用
    //QString arra[248][328], arrb[248][328], arrc[248][328], arrd[248][328], arre[248][328], arrf[248][328], arrg[248][328], arrh[248][328];
    QString xAxis, yAxis; //軸の名前
    QString databPath;
    QVector<double> vx, vy; //グラフ描画用　それぞれ対になるように格納 例 vx={0,1,2} vy={0,1,2}の場合だと点(0,0)(1,1)(2,2)が描画される
    bool axisFlag=false; //軸の初期設定に使用
    int infoNum=0; //
    int previousNum=0; //初期段階の総数
    int plotNum=0; //リプロットで絞り込まれた数
    int regressionGraphCount=1; // 描画されているグラフの数
    double xMax,xMin,yMax,yMin, xRangeMax, xRangeMin, yRangeMax, yRangeMin;
    double tgtMax,tgtMin, boloMax, boloMin, pkgMax, pkgMin, caseMax, caseMin, shMax, shMin, lensMax, lensMin;
    double a, b, c, d, e, f, g, h; //回帰係数
    double planck(double T);
    void OutputSliderValue(QString);
    void OutputUsedImage(QString);
    double OutputSliderValuearray[20];
    //Ui::CalibrationGraph *ui;

private:
    Ui::CalibrationGraph *ui;
    QVector<double> getAxisValue(QString axis, QVector<QVector<QString> > info, int itemNum);
    QVector<double> getAxisValue2(QString axis, QVector<QVector<QString> > info, int itemNum);
    QSqlDatabase db;
    QSqlQuery query;

    void drawGraph(QVector<double> x, QVector<double> y, QString lineType);
    int judgeAxis(QString x, QString y);
    void setRegressionCoefficient(QVector<double> vx, QVector<double> vy);
    void setRegressionCoefficientforBlack(QVector<double> vx, QVector<double> vy);
    void closeEvent(QCloseEvent *e);
    void setInitializeUI();
    void setMax();
    void setMin();
    void connectDB();
    QDir filterpath;
    QString judgeTableName(int x, int y);
        int judgeTableNameint(int x, int y);
    QString getRegressionCoefficientInRegisterRegression(QVector<double> vx, QVector<double> vy, int xc, int yc);
    QString getRegressionCoefficientInRegisterRegressionforBlackbody(QVector<double> vx, QVector<double> vy, int xc, int yc);
    QString dataPath;
    void saveFileInRegisterRegression(QString fileName, QString folder);
    //  double planck(double T);
    void loadFilter();
    double h_planck;
    double kB;
    double c_speed;
    double c1;
    double c2;
    double SIGMA;
    // double tirfilter[2000][3];
    //QString Usedimage[4000];
    //QString fitsfilename;
    //QString ImageFilefilepath;

    int Outputplotnumberini;
    int Outputplotnumber;
    int numfortxt;


public slots:
    void popCalibrationGraph(QVector< QVector<QString> > p, QString xAxisName, QString yAxisName, QString lineType, int n, bool ismodified);

private slots:
    void on_regressionButton_clicked();
    void on_tgtMaxSlider_valueChanged(int value);
    void on_tgtMinSlider_valueChanged(int value);
    void on_boloMaxSlider_valueChanged(int value);
    void on_boloMinSlider_valueChanged(int value);
    void on_pkgMaxSlider_valueChanged(int value);
    void on_pkgMinSlider_valueChanged(int value);
    void on_caseMaxSlider_valueChanged(int value);
    void on_caseMinSlider_valueChanged(int value);
    void on_shMaxSlider_valueChanged(int value);
    void on_shMinSlider_valueChanged(int value);
    void on_lensMaxSlider_valueChanged(int value);
    void on_lensMinSlider_valueChanged(int value);
    void on_replotButton_clicked();
    void on_tgtMaxLineEdit_textChanged(const QString &arg1);
    void on_tgtMinLineEdit_textChanged(const QString &arg1);
    void on_boloMaxLineEdit_textChanged(const QString &arg1);
    void on_boloMinLineEdit_textChanged(const QString &arg1);
    void on_pkgMaxLineEdit_textChanged(const QString &arg1);
    void on_pkgMinLineEdit_textChanged(const QString &arg1);
    void on_caseMaxLineEdit_textChanged(const QString &arg1);
    void on_caseMinLineEdit_textChanged(const QString &arg1);
    void on_shMaxLineEdit_textChanged(const QString &arg1);
    void on_shMinLineEdit_textChanged(const QString &arg1);
    void on_lensMaxLineEdit_textChanged(const QString &arg1);
    void on_lensMinLineEdit_textChanged(const QString &arg1);
    void on_outputCSVFileButton_clicked();
    void on_outputGraphImageButton_clicked();
    void contextMenuRequest(QPoint pos);
    void graphClicked(QCPAbstractPlottable *);
    void removeSelectedGraph();
    void removeRegressionAllGraphs();
    void mousePress();
    void on_plotFormulaButton_clicked();
    void on_exportFormulaButton_clicked();
    void on_loadFileButton_clicked();
    void getDataPath(QString);
};

#endif // CALIBRATIONGRAPH_H
