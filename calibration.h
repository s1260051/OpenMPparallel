#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <QDialog>
#include <QtSql>
#include <QProgressDialog>
#include <QString>
#include <algorithm>
#include <QFileDialog>
#include "database.h"
#include <QMessageBox>

namespace Ui {
class Calibration;
}

class Calibration : public QDialog
{
    Q_OBJECT

public:
    explicit Calibration(QWidget *parent = 0);
    ~Calibration();
    Ui::Calibration *ui;
    QSqlDatabase db;
    QSqlQuery query; //データの最大値、最小値の設定用
    QSqlQuery pixelQuery; //ピクセル情報の本体
    QString info[20]; //インフォメーションの記述用
    QVector< QVector<QString> > pixelList; //条件合致したピクセル情報の格納用
    QIntValidator *XValidator, *YValidator; //座標入力範囲設定用
    int queryNum=0; //条件合致したピクセルの数
    int px=-1, py=-1; //直前で検索した座標　ちなみにpはpreviousの略です
    double T_max=-1000,T_min=1000,B_max=0,B_min=100,P_max=0,P_min=100,C_max=0,C_min=100,S_max=0,S_min=100,L_max=0,L_min=100; //それぞれのパラメータの最大値、最小値用
    bool coordinateChangeFlag=false; //座標の値を変更したのにサーチボタンを押さないでグラフを表示しようとした時の注意する用

    QVector<QVector<QString>> getCoffecientAndOffset(QString fileName);
    QString dataPath;

private:
    void connectDB();
    void fillForm();
    void setValue();
    QString nameChange(QString tmp);
    bool judgeItem();
    QString judgeTableName(int x, int y);

signals:
    void showCalibrationGraphSignal(QVector< QVector<QString> >, QString, QString, QString, int, bool);
    void showControlGraphPanel(QString**);

public slots:
    void checkAction();
    void getDataPath(QString);
    void setX(QString x);
    void setY(QString y);

private slots:
    void on_searchPixcelButton_clicked();
    void on_pixelList_clicked(const QModelIndex &index);
    void on_showGraphButton_clicked();
    void showCalibrationPanel();
    void on_x_textChanged(const QString &arg1);
    void on_y_textChanged(const QString &arg1);
};

#endif // CALIBRATION_H
