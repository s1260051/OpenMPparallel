#include "calibrationgraph.h"
#include "ui_calibrationgraph.h"
#include <iostream>
#include <fstream>
#include <QDateTime>
#include <showimage.h>
#include <calibration.h>
#include <controlgraphpanel.h>
#include <exception>
#include <thread>
#include <exception>
#include <QtConcurrent>
#include <omp.h>
QVector<QVector<QString>> CreateTableau(int sizeX, int sizeY)
{
   QVector<QVector<QString>> result;
   for (int idx1 = 0; idx1 < sizeX; idx1++)
   {
      result.append(QVector<QString>());
      for (int idx2 = 0; idx2 < sizeY; idx2++)
      {
         result[idx1].append(QString());
      } 
   }
   return result;
}

int xPosition[10000], yPosition[10000], isActive[10000]; // １がActive ０がnot Active
int n = 0, sum = 0;
QString Usedimage[4000];
double tirfilter[2000][3];

double bol_temp;
double pkg_temp;
double cas_temp;
double sht_temp;
double len_temp;
bool ismodifiedgl;

QString fitsfilename;
QString ImageFilefilepath;

#define Width 384
#define Height 256

using namespace std;
int pixelxy_thread;
int Outputplotnumber_thread;
QVector<QVector<QString>> tmp2_thread;
QString xAxis_thread;
QString yAxis_thread;
QString initialFileDirectory_thread;
int xaxis_thread[1024];
int yaxis_thread[1024];
QString num2_thread;
QString residual[248][328]; //画像毎登録の際に残差をテキストファイルを作成する用
QString arra[248][328], arrb[248][328], arrc[248][328], arrd[248][328], arre[248][328], arrf[248][328], arrg[248][328], arrh[248][328];


void ForThread1::run()
{

    //    必要
    //  pixelxy,outputplotnumber,tmp2,xAxis,yAxis
    // getRegressionCoefficientInRegisterRegressionforBlackbody()
    int searchID, pairID;
    int xycounter = 0;
    QVector<QString> t;
    QVector<QVector<QString>> tmp3;
    QDir tmp5; //データパス用
    /*
    qDebug()<<pixelxy_thread;
    qDebug()<<Outputplotnumber_thread;
    qDebug()<<xAxis_thread;
    qDebug()<<yAxis_thread;
    qDebug()<<num2_thread;
    qDebug()<<initialFileDirectory_thread;
*/
    //   qDebug()<<tmp2_thread;
    //qDebug() << "thread1";

    for (int pix = 0; pix <pixelxy_thread*0.166; pix++)
    {
        //qDebug()<<"<<pix;
        for (int plotn = 0; plotn < Outputplotnumber_thread * 2; plotn++)
        {
            //qDebug()<<plotn;
            //qDebug()<<"<plotn"<<plotn;
            //  qDebug()<<tmp2[pixelxy*plotn+pix];//lをpixelxy回回すと同一ピクセルにもどる
            int currentpixel = pixelxy_thread * plotn + pix;
            if (tmp2_thread[currentpixel][0].section('.', 2, 2) == "open" && tmp2_thread[currentpixel][6]=="1")
            {
                // qDebug()<<"mask1";
                searchID = tmp2_thread[currentpixel][3].toInt(0, 16);
                //IDが奇数のときの処理
                if (searchID % 2)
                {
                    pairID = searchID + 1;
                }
                else
                {
                    //IDが偶数の時の処理
                    pairID = searchID - 1;
                }
                //qDebug()<<"ghk";
                //基本はopenの情報を格納しつつ、DN値のみ差分を格納
                for (int plotn2 = 0; plotn2 < Outputplotnumber_thread * 2; plotn2++)
                {
                    //     qDebug()<<"plotn2"<<plotn2;
                    int currentpixel2 = pixelxy_thread * plotn2 + pix;
                    if (tmp2_thread[currentpixel2][3].toInt(0, 16) == pairID && tmp2_thread[currentpixel][4] == tmp2_thread[currentpixel2][4] && tmp2_thread[currentpixel][5] == tmp2_thread[currentpixel2][5])
                    {
                        t.clear();
                        t.append(tmp2_thread[pixelxy_thread * plotn + pix][0]);
                        if (xAxis_thread == "diff DN")
                        {
                            t.append(QString::number(tmp2_thread[currentpixel][1].toDouble() - tmp2_thread[currentpixel2][1].toDouble()));
                            t.append(tmp2_thread[pixelxy_thread * plotn + pix][2]);
                        }
                        else if (yAxis_thread == "diff DN")
                        {
                            t.append(tmp2_thread[pixelxy_thread * plotn + pix][1]);
                            t.append(QString::number(tmp2_thread[currentpixel][2].toDouble() - tmp2_thread[currentpixel2][2].toDouble()));
                        }
                        tmp3.append(t);
                        break;
                    }
                }
            }                                //qDebug()<<"mask0";
        }
//qDebug()<<"mask10513532";
        // qDebug()<<tmp3;

        //ファイル作成処理
        //それぞれvx、vyに格納
        QVector<double> vx;
        QVector<double> vy;
        if (yAxis_thread == "diff DN" || xAxis_thread == "diff DN")
        {
            for (int l = 0; l < tmp3.size(); l++)
            {
                vx.append(tmp3[l][1].toDouble());
                vy.append(tmp3[l][2].toDouble());
            }
        }
        else
        {
            for (int l = 0; l < tmp2_thread.size(); l++)
            {
                vx.append(tmp2_thread[l][1].toDouble());
                vy.append(tmp2_thread[l][2].toDouble());
            }
        }
        cout<<"(";cout<<xaxis_thread[xycounter] - 16;cout<<",";cout<<yaxis_thread[xycounter] - 6;cout<<")";


        QString s;
        //回帰係数を計算し、登録の際に使用するSringを取得
        //   s = getRegressionCoefficientInRegisterRegressionforBlackbody(vx, vy, xaxis_thread[xycounter]-16, yaxis_thread[xycounter]-6);

        int num;
        double x = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0, x6 = 0, x7 = 0, x8 = 0, x9 = 0, x10 = 0, x11 = 0, x12 = 0, x13 = 0, x14 = 0;
        double x7y = 0, x6y = 0, x5y = 0, x4y = 0, x3y = 0, x2y = 0, xy = 0, y = 0;
        double pivot, mul;
        double aa, bb, cc, dd, ee, ff, gg, hh;
        QString coefficient, formula;
        //次数の取得 & +1の処理
        //num = ui->widget->selectedGraphs().at(0)->name().section(",",0,0).toInt() + 1;
        num = 2;
        //qDebug() << num;

        //係数の初期化
        aa = bb = cc = dd = ee = ff = gg = hh = 0;

        //それぞれの総和
        for (int i = 0; i < vx.size(); i++)
        {
            x += planck4(vx[i] + 273.15);
            x2 += pow(planck4(vx[i] + 273.15), 2);
            x3 += pow(planck4(vx[i] + 273.15), 3);
            x4 += pow(planck4(vx[i] + 273.15), 4);
            x5 += pow(planck4(vx[i] + 273.15), 5);
            x6 += pow(planck4(vx[i] + 273.15), 6);
            x7 += pow(planck4(vx[i] + 273.15), 7);
            x8 += pow(planck4(vx[i] + 273.15), 8);
            x9 += pow(planck4(vx[i] + 273.15), 9);
            x10 += pow(planck4(vx[i] + 273.15), 10);
            x11 += pow(planck4(vx[i] + 273.15), 11);
            x12 += pow(planck4(vx[i] + 273.15), 12);
            x13 += pow(planck4(vx[i] + 273.15), 13);
            x14 += pow(planck4(vx[i] + 273.15), 14);
            x7y += (pow(planck4(vx[i] + 273.15), 7) * vy[i]);
            x6y += (pow(planck4(vx[i] + 273.15), 6) * vy[i]);
            x5y += (pow(planck4(vx[i] + 273.15), 5) * vy[i]);
            x4y += (pow(planck4(vx[i] + 273.15), 4) * vy[i]);
            x3y += (pow(planck4(vx[i] + 273.15), 3) * vy[i]);
            x2y += (pow(planck4(vx[i] + 273.15), 2) * vy[i]);
            xy += (planck4(vx[i] + 273.15) * vy[i]);
            y += vy[i];
            /*   cout<<"T"<<endl;
                                         cout<<vx[i]<<endl;

                                            cout<<"F"<<endl;
                                            cout<<planck(vx[i]+273.15)<<endl;
                                    */
        }

        // 拡大係数行列 M
        double M[8][8 + 1] =
        {
            {x14, x13, x12, x11, x10, x9, x8, x7, x7y},
            {x13, x12, x11, x10, x9, x8, x7, x6, x6y},
            {x12, x11, x10, x9, x8, x7, x6, x5, x5y},
            {x11, x10, x9, x8, x7, x6, x5, x4, x4y},
            {x10, x9, x8, x7, x6, x5, x4, x3, x3y},
            {x9, x8, x7, x6, x5, x4, x3, x2, x2y},
            {x8, x7, x6, x5, x4, x3, x2, x, xy},
            {x7, x6, x5, x4, x3, x2, x, double(vx.size()), y}};

        double **M2 = new double *[num];
        for (int i = 0; i < num; i++)
        {
            M2[i] = new double[num + 1];
        }

        for (int i = 0; i < num; i++)
        {
            for (int j = 0; j < num + 1; j++)
            {
                M2[i][j] = M[(8 - num) + i][(8 - num) + j];
            }
        }

        // 対角成分が1で正規化された階段行列を作る(前進消去)
        for (int i = 0; i < num; ++i)
        {
            // 対角成分の選択、この値で行成分を正規化
            pivot = M2[i][i];
            for (int j = 0; j < num + 1; ++j)
            {
                M2[i][j] = (1 / pivot) * M2[i][j];
            }

            // 階段行列を作る為に、現在の行より下の行について
            // i列目の成分が0になるような基本変形をする
            for (int k = i + 1; k < num; ++k)
            {
                mul = M2[k][i];
                for (int count = i; count < num + 1; ++count)
                {
                    M2[k][count] = M2[k][count] - mul * M2[i][count];
                }
            }
        }

        // 下から上に向かって変数に代入して、独立した解の形にする(後進代入)
        // このとき一番下の行はすでに独立した解を得ている
        for (int i = num - 1; i > 0; --i)
        {
            for (int k = i - 1; k >= 0; --k)
            {
                mul = M2[k][i];
                for (int count = i; count < num + 1; ++count)
                {
                    M2[k][count] = M2[k][count] - mul * M2[i][count];
                }
            }
        }

        //対応する係数を格納
        if (1 < num)
        {
            hh = M2[num - 1][num];
            gg = M2[num - 2][num];
        }
        if (2 < num)
        {
            ff = M2[num - 3][num];
        }
        if (3 < num)
        {
            ee = M2[num - 4][num];
        }
        if (4 < num)
        {
            dd = M2[num - 5][num];
        }
        if (5 < num)
        {
            cc = M2[num - 6][num];
        }
        if (6 < num)
        {
            bb = M2[num - 7][num];
        }
        if (7 < num)
        {
            aa = M2[num - 8][num];
        }

        //メモリの解放
        for (int i = 0; i < num; i++)
        {
            delete[] M2[i];
        }
        delete[] M2;

        //残差を求める
        double d = 0;
        double tmp;
        for (int i = 0; i < vx.size(); i++)
        {
            tmp = vy[i] - (gg * vx[i] + hh);
            if (tmp < 0)
            {
                d += tmp * -1;
            }
            else
            {
                d += tmp;
            }
        }
        d = d / vx.size();
        //残差配列に座標に対応する残差を格納
        residual[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(d);
        arra[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(aa);
        arrb[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(bb);
        arrc[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(cc);
        arrd[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(dd);
        arre[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(ee);
        arrf[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(ff);
        arrg[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(gg);
        arrh[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(hh);

        saveFileInRegisterRegression("residual.csv",initialFileDirectory_thread);

        saveFileInRegisterRegression("a.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("b.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("c.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("d.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("e.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("f.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("g.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("h.csv",initialFileDirectory_thread);
        //ファイルに記述する際に使用する文を作成

        coefficient.append(QString::number(aa) + ",");
        coefficient.append(QString::number(bb) + ",");
        coefficient.append(QString::number(cc) + ",");
        coefficient.append(QString::number(dd) + ",");
        coefficient.append(QString::number(ee) + ",");
        coefficient.append(QString::number(ff) + ",");
        coefficient.append(QString::number(gg) + ",");
        coefficient.append(QString::number(hh) + ",");

        num = num - 1;

        if (2 == num)
        {
            formula.append(QString::number(ff) + "x^2 ");
        }
        else if (2 <= num)
        {
            if (ff < 0)
            {
                formula.append("- " + QString::number(-1 * ff) + "x^2 ");
            }
            else
            {
                formula.append("+ " + QString::number(ff) + "x^2 ");
            }
        }

        if (1 == num || num2_thread == "Black_Body")
        {
            if (num2_thread == "Black_Body")
                formula.append(QString::number(gg) + "*F( T + 273.15) ");
            else
                formula.append(QString::number(gg) + "x ");
        }
        else if (1 <= num)
        {
            if (gg < 0)
            {
                if (num2_thread == "Black_Body")
                    formula.append("- " + QString::number(-1 * gg) + "*F( T + 273.15) ");
                else
                    formula.append("- " + QString::number(-1 * gg) + "x ");
            }
            else
            {
                if (num2_thread == "Black_Body")
                    formula.append("+ " + QString::number(gg) + "*F( T + 273.15) ");
                else
                    formula.append("+ " + QString::number(gg) + "x ");
            }
        }


        if (hh < 0)
        {
            formula.append("- " + QString::number(-1 * hh));
        }
        else
        {
            formula.append("+ " + QString::number(hh));
        }

        s = QString::number(num) + "," + coefficient + formula;

        //取得した登録内容をファイルに書き込み
        QFile file(initialFileDirectory_thread + "/" + QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");
        vx.clear();
        vy.clear();
        tmp3.clear();
        //ファイルが存在する場合
        if (file.exists())
        {
            file.open(QIODevice::WriteOnly);
            QTextStream out(&file);

            out << s << endl;

            file.close();
        }
        else
        { //ファイルが存在しなかった場合
            //ファイル作成
            //QFile ini(QString::number(x-16) + "_" + QString::number(y-6) + "_" + tmp1[l]+ ".txt");
            QFile ini(QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");

            ini.open(QIODevice::WriteOnly);

            QTextStream out(&ini);

            out << s << endl;

            ini.close();

            tmp5.rename(QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt",
                        initialFileDirectory_thread + "/" + QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");
        }
        xycounter++;
        tmp3.clear();
    }
}

void ForThread2::run()
{
    int searchID, pairID;
    int xycounter = pixelxy_thread*0.166;
    QVector<QString> t;
    QVector<QVector<QString>> tmp3;
    QDir tmp5; //データパス用

    //   qDebug()<<tmp2_thread;
    //qDebug() << "thread2";

    for (int pix = pixelxy_thread*0.166; pix < pixelxy_thread*0.166*2; pix++)
    {
        //    qDebug()<<"<<pix;
        for (int plotn = 0; plotn < Outputplotnumber_thread * 2; plotn++)
        {
            //qDebug()<<"<plotn"<<plotn;
            //  qDebug()<<tmp2[pixelxy*plotn+pix];//lをpixelxy回回すと同一ピクセルにもどる
            int currentpixel = pixelxy_thread * plotn + pix;
            //qDebug()<<plotn;
            if (tmp2_thread[currentpixel][0].section('.', 2, 2) == "open"&& tmp2_thread[currentpixel][6]=="1")
            {
                //qDebug()<<"def";
                searchID = tmp2_thread[currentpixel][3].toInt(0, 16);
                //IDが奇数のときの処理
                if (searchID % 2)
                {
                    pairID = searchID + 1;
                }
                else
                {
                    //IDが偶数の時の処理
                    pairID = searchID - 1;
                }
            //    qDebug()<<"ghk";
                //基本はopenの情報を格納しつつ、DN値のみ差分を格納
                for (int plotn2 = 0; plotn2 < Outputplotnumber_thread * 2; plotn2++)
                {
                    //     qDebug()<<"plotn2"<<plotn2;
                    int currentpixel2 = pixelxy_thread * plotn2 + pix;
                    if (tmp2_thread[currentpixel2][3].toInt(0, 16) == pairID && tmp2_thread[currentpixel][4] == tmp2_thread[currentpixel2][4] && tmp2_thread[currentpixel][5] == tmp2_thread[currentpixel2][5])
                    {
                        t.clear();
                        t.append(tmp2_thread[pixelxy_thread * plotn + pix][0]);
                        if (xAxis_thread == "diff DN")
                        {
                            t.append(QString::number(tmp2_thread[currentpixel][1].toDouble() - tmp2_thread[currentpixel2][1].toDouble()));
                            t.append(tmp2_thread[pixelxy_thread * plotn + pix][2]);
                        }
                        else if (yAxis_thread == "diff DN")
                        {
                            t.append(tmp2_thread[pixelxy_thread * plotn + pix][1]);
                            t.append(QString::number(tmp2_thread[currentpixel][2].toDouble() - tmp2_thread[currentpixel2][2].toDouble()));
                        }
                        tmp3.append(t);
                        break;
                    }
                }
            }            //                qDebug()<<"mask0";
        }
//qDebug()<<"mask10513532";
        //  qDebug()<<tmp3;

        //ファイル作成処理
        //それぞれvx、vyに格納
        QVector<double> vx;
        QVector<double> vy;
        if (yAxis_thread == "diff DN" || xAxis_thread == "diff DN")
        {
            for (int l = 0; l < tmp3.size(); l++)
            {
                vx.append(tmp3[l][1].toDouble());
                vy.append(tmp3[l][2].toDouble());
            }
        }
        else
        {
            for (int l = 0; l < tmp2_thread.size(); l++)
            {
                vx.append(tmp2_thread[l][1].toDouble());
                vy.append(tmp2_thread[l][2].toDouble());
            }
        }
        cout<<"(";cout<<xaxis_thread[xycounter] - 16;cout<<",";cout<<yaxis_thread[xycounter] - 6;cout<<")";


        QString s;
        //回帰係数を計算し、登録の際に使用するSringを取得
        //   s = getRegressionCoefficientInRegisterRegressionforBlackbody(vx, vy, xaxis_thread[xycounter]-16, yaxis_thread[xycounter]-6);

        int num;
        double x = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0, x6 = 0, x7 = 0, x8 = 0, x9 = 0, x10 = 0, x11 = 0, x12 = 0, x13 = 0, x14 = 0;
        double x7y = 0, x6y = 0, x5y = 0, x4y = 0, x3y = 0, x2y = 0, xy = 0, y = 0;
        double pivot, mul;
        double aa, bb, cc, dd, ee, ff, gg, hh;
        QString coefficient, formula;
        //次数の取得 & +1の処理
        //num = ui->widget->selectedGraphs().at(0)->name().section(",",0,0).toInt() + 1;
        num = 2;
        //qDebug() << num;

        //係数の初期化
        aa = bb = cc = dd = ee = ff = gg = hh = 0;

        //それぞれの総和
        for (int i = 0; i < vx.size(); i++)
        {
            x += planck4(vx[i] + 273.15);
            x2 += pow(planck4(vx[i] + 273.15), 2);
            x3 += pow(planck4(vx[i] + 273.15), 3);
            x4 += pow(planck4(vx[i] + 273.15), 4);
            x5 += pow(planck4(vx[i] + 273.15), 5);
            x6 += pow(planck4(vx[i] + 273.15), 6);
            x7 += pow(planck4(vx[i] + 273.15), 7);
            x8 += pow(planck4(vx[i] + 273.15), 8);
            x9 += pow(planck4(vx[i] + 273.15), 9);
            x10 += pow(planck4(vx[i] + 273.15), 10);
            x11 += pow(planck4(vx[i] + 273.15), 11);
            x12 += pow(planck4(vx[i] + 273.15), 12);
            x13 += pow(planck4(vx[i] + 273.15), 13);
            x14 += pow(planck4(vx[i] + 273.15), 14);
            x7y += (pow(planck4(vx[i] + 273.15), 7) * vy[i]);
            x6y += (pow(planck4(vx[i] + 273.15), 6) * vy[i]);
            x5y += (pow(planck4(vx[i] + 273.15), 5) * vy[i]);
            x4y += (pow(planck4(vx[i] + 273.15), 4) * vy[i]);
            x3y += (pow(planck4(vx[i] + 273.15), 3) * vy[i]);
            x2y += (pow(planck4(vx[i] + 273.15), 2) * vy[i]);
            xy += (planck4(vx[i] + 273.15) * vy[i]);
            y += vy[i];
            /*   cout<<"T"<<endl;
                                         cout<<vx[i]<<endl;

                                            cout<<"F"<<endl;
                                            cout<<planck(vx[i]+273.15)<<endl;
                                    */
        }

        // 拡大係数行列 M
        double M[8][8 + 1] =
        {
            {x14, x13, x12, x11, x10, x9, x8, x7, x7y},
            {x13, x12, x11, x10, x9, x8, x7, x6, x6y},
            {x12, x11, x10, x9, x8, x7, x6, x5, x5y},
            {x11, x10, x9, x8, x7, x6, x5, x4, x4y},
            {x10, x9, x8, x7, x6, x5, x4, x3, x3y},
            {x9, x8, x7, x6, x5, x4, x3, x2, x2y},
            {x8, x7, x6, x5, x4, x3, x2, x, xy},
            {x7, x6, x5, x4, x3, x2, x, double(vx.size()), y}};

        double **M2 = new double *[num];
        for (int i = 0; i < num; i++)
        {
            M2[i] = new double[num + 1];
        }

        for (int i = 0; i < num; i++)
        {
            for (int j = 0; j < num + 1; j++)
            {
                M2[i][j] = M[(8 - num) + i][(8 - num) + j];
            }
        }

        // 対角成分が1で正規化された階段行列を作る(前進消去)
        for (int i = 0; i < num; ++i)
        {
            // 対角成分の選択、この値で行成分を正規化
            pivot = M2[i][i];
            for (int j = 0; j < num + 1; ++j)
            {
                M2[i][j] = (1 / pivot) * M2[i][j];
            }

            // 階段行列を作る為に、現在の行より下の行について
            // i列目の成分が0になるような基本変形をする
            for (int k = i + 1; k < num; ++k)
            {
                mul = M2[k][i];
                for (int count = i; count < num + 1; ++count)
                {
                    M2[k][count] = M2[k][count] - mul * M2[i][count];
                }
            }
        }

        // 下から上に向かって変数に代入して、独立した解の形にする(後進代入)
        // このとき一番下の行はすでに独立した解を得ている
        for (int i = num - 1; i > 0; --i)
        {
            for (int k = i - 1; k >= 0; --k)
            {
                mul = M2[k][i];
                for (int count = i; count < num + 1; ++count)
                {
                    M2[k][count] = M2[k][count] - mul * M2[i][count];
                }
            }
        }

        //対応する係数を格納
        if (1 < num)
        {
            hh = M2[num - 1][num];
            gg = M2[num - 2][num];
        }
        if (2 < num)
        {
            ff = M2[num - 3][num];
        }
        if (3 < num)
        {
            ee = M2[num - 4][num];
        }
        if (4 < num)
        {
            dd = M2[num - 5][num];
        }
        if (5 < num)
        {
            cc = M2[num - 6][num];
        }
        if (6 < num)
        {
            bb = M2[num - 7][num];
        }
        if (7 < num)
        {
            aa = M2[num - 8][num];
        }

        //メモリの解放
        for (int i = 0; i < num; i++)
        {
            delete[] M2[i];
        }
        delete[] M2;

        //残差を求める
        double d = 0;
        double tmp;
        for (int i = 0; i < vx.size(); i++)
        {
            tmp = vy[i] - (gg * vx[i] + hh);
            if (tmp < 0)
            {
                d += tmp * -1;
            }
            else
            {
                d += tmp;
            }
        }
        d = d / vx.size();
        //残差配列に座標に対応する残差を格納
        residual[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(d);
        arra[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(aa);
        arrb[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(bb);
        arrc[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(cc);
        arrd[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(dd);
        arre[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(ee);
        arrf[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(ff);
        arrg[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(gg);
        arrh[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(hh);

        saveFileInRegisterRegression("residual.csv",initialFileDirectory_thread);

        saveFileInRegisterRegression("a.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("b.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("c.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("d.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("e.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("f.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("g.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("h.csv",initialFileDirectory_thread);
        //ファイルに記述する際に使用する文を作成

        coefficient.append(QString::number(aa) + ",");
        coefficient.append(QString::number(bb) + ",");
        coefficient.append(QString::number(cc) + ",");
        coefficient.append(QString::number(dd) + ",");
        coefficient.append(QString::number(ee) + ",");
        coefficient.append(QString::number(ff) + ",");
        coefficient.append(QString::number(gg) + ",");
        coefficient.append(QString::number(hh) + ",");

        num = num - 1;

        if (2 == num)
        {
            formula.append(QString::number(ff) + "x^2 ");
        }
        else if (2 <= num)
        {
            if (ff < 0)
            {
                formula.append("- " + QString::number(-1 * ff) + "x^2 ");
            }
            else
            {
                formula.append("+ " + QString::number(ff) + "x^2 ");
            }
        }

        if (1 == num || num2_thread == "Black_Body")
        {
            if (num2_thread == "Black_Body")
                formula.append(QString::number(gg) + "*F( T + 273.15) ");
            else
                formula.append(QString::number(gg) + "x ");
        }
        else if (1 <= num)
        {
            if (gg < 0)
            {
                if (num2_thread == "Black_Body")
                    formula.append("- " + QString::number(-1 * gg) + "*F( T + 273.15) ");
                else
                    formula.append("- " + QString::number(-1 * gg) + "x ");
            }
            else
            {
                if (num2_thread == "Black_Body")
                    formula.append("+ " + QString::number(gg) + "*F( T + 273.15) ");
                else
                    formula.append("+ " + QString::number(gg) + "x ");
            }
        }

        if (hh < 0)
        {
            formula.append("- " + QString::number(-1 * hh));
        }
        else
        {
            formula.append("+ " + QString::number(hh));
        }

        s = QString::number(num) + "," + coefficient + formula;

        //取得した登録内容をファイルに書き込み
        QFile file(initialFileDirectory_thread + "/" + QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");
        vx.clear();
        vy.clear();
        tmp3.clear();
        //ファイルが存在する場合
        if (file.exists())
        {
            file.open(QIODevice::WriteOnly);
            QTextStream out(&file);

            out << s << endl;

            file.close();
        }
        else
        { //ファイルが存在しなかった場合
            //ファイル作成
            //QFile ini(QString::number(x-16) + "_" + QString::number(y-6) + "_" + tmp1[l]+ ".txt");
            QFile ini(QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");

            ini.open(QIODevice::WriteOnly);

            QTextStream out(&ini);

            out << s << endl;

            ini.close();

            tmp5.rename(QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt",
                        initialFileDirectory_thread + "/" + QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");
        }
        xycounter++;
        tmp3.clear();
    }
}

void ForThread3::run()
{
    int searchID, pairID;
    int xycounter = pixelxy_thread*0.166*2;
    QVector<QString> t;
    QVector<QVector<QString>> tmp3;
    QDir tmp5; //データパス用

    //   qDebug()<<tmp2_thread;
    qDebug() << "thread3";
    for (int pix = pixelxy_thread*0.166*2; pix < pixelxy_thread *0.5; pix++)
    {
        //    qDebug()<<"pix"<<pix;
        for (int plotn = 0; plotn < Outputplotnumber_thread * 2; plotn++)
        {
            //qDebug()<<"<plotn"<<plotn;
            //  qDebug()<<tmp2[pixelxy*plotn+pix];//lをpixelxy回回すと同一ピクセルにもどる
            int currentpixel = pixelxy_thread * plotn + pix;
           // qDebug()<<plotn;
            if (tmp2_thread[currentpixel][0].section('.', 2, 2) == "open"&& tmp2_thread[currentpixel][6]=="1")
            {
            //    qDebug()<<"def";
                searchID = tmp2_thread[currentpixel][3].toInt(0, 16);
                //IDが奇数のときの処理
                if (searchID % 2)
                {
                    pairID = searchID + 1;
                }
                else
                {
                    //IDが偶数の時の処理
                    pairID = searchID - 1;
                }
              //  qDebug()<<"ghk";
                //基本はopenの情報を格納しつつ、DN値のみ差分を格納
                for (int plotn2 = 0; plotn2 < Outputplotnumber_thread * 2; plotn2++)
                {
                    //     qDebug()<<"plotn2"<<plotn2;
                    int currentpixel2 = pixelxy_thread * plotn2 + pix;
                    if (tmp2_thread[currentpixel2][3].toInt(0, 16) == pairID && tmp2_thread[currentpixel][4] == tmp2_thread[currentpixel2][4] && tmp2_thread[currentpixel][5] == tmp2_thread[currentpixel2][5])
                    {
                        t.clear();
                        t.append(tmp2_thread[pixelxy_thread * plotn + pix][0]);
                        if (xAxis_thread == "diff DN")
                        {
                            t.append(QString::number(tmp2_thread[currentpixel][1].toDouble() - tmp2_thread[currentpixel2][1].toDouble()));
                            t.append(tmp2_thread[pixelxy_thread * plotn + pix][2]);
                        }
                        else if (yAxis_thread == "diff DN")
                        {
                            t.append(tmp2_thread[pixelxy_thread * plotn + pix][1]);
                            t.append(QString::number(tmp2_thread[currentpixel][2].toDouble() - tmp2_thread[currentpixel2][2].toDouble()));
                        }
                        tmp3.append(t);
                        break;
                    }
                }
            }                         //        qDebug()<<"mask0";
        }
// qDebug()<<"mask10513532";
        //  qDebug()<<tmp3;

        //ファイル作成処理
        //それぞれvx、vyに格納
        QVector<double> vx;
        QVector<double> vy;
        if (yAxis_thread == "diff DN" || xAxis_thread == "diff DN")
        {
            for (int l = 0; l < tmp3.size(); l++)
            {
                vx.append(tmp3[l][1].toDouble());
                vy.append(tmp3[l][2].toDouble());
            }
        }
        else
        {
            for (int l = 0; l < tmp2_thread.size(); l++)
            {
                vx.append(tmp2_thread[l][1].toDouble());
                vy.append(tmp2_thread[l][2].toDouble());
            }
        }

        cout<<"(";cout<<xaxis_thread[xycounter] - 16;cout<<",";cout<<yaxis_thread[xycounter] - 6;cout<<")";


        QString s;
        //回帰係数を計算し、登録の際に使用するSringを取得
        //   s = getRegressionCoefficientInRegisterRegressionforBlackbody(vx, vy, xaxis_thread[xycounter]-16, yaxis_thread[xycounter]-6);

        int num;
        double x = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0, x6 = 0, x7 = 0, x8 = 0, x9 = 0, x10 = 0, x11 = 0, x12 = 0, x13 = 0, x14 = 0;
        double x7y = 0, x6y = 0, x5y = 0, x4y = 0, x3y = 0, x2y = 0, xy = 0, y = 0;
        double pivot, mul;
        double aa, bb, cc, dd, ee, ff, gg, hh;
        QString coefficient, formula;
        //次数の取得 & +1の処理
        //num = ui->widget->selectedGraphs().at(0)->name().section(",",0,0).toInt() + 1;
        num = 2;
        //qDebug() << num;

        //係数の初期化
        aa = bb = cc = dd = ee = ff = gg = hh = 0;

        //それぞれの総和
        for (int i = 0; i < vx.size(); i++)
        {
            x += planck4(vx[i] + 273.15);
            x2 += pow(planck4(vx[i] + 273.15), 2);
            x3 += pow(planck4(vx[i] + 273.15), 3);
            x4 += pow(planck4(vx[i] + 273.15), 4);
            x5 += pow(planck4(vx[i] + 273.15), 5);
            x6 += pow(planck4(vx[i] + 273.15), 6);
            x7 += pow(planck4(vx[i] + 273.15), 7);
            x8 += pow(planck4(vx[i] + 273.15), 8);
            x9 += pow(planck4(vx[i] + 273.15), 9);
            x10 += pow(planck4(vx[i] + 273.15), 10);
            x11 += pow(planck4(vx[i] + 273.15), 11);
            x12 += pow(planck4(vx[i] + 273.15), 12);
            x13 += pow(planck4(vx[i] + 273.15), 13);
            x14 += pow(planck4(vx[i] + 273.15), 14);
            x7y += (pow(planck4(vx[i] + 273.15), 7) * vy[i]);
            x6y += (pow(planck4(vx[i] + 273.15), 6) * vy[i]);
            x5y += (pow(planck4(vx[i] + 273.15), 5) * vy[i]);
            x4y += (pow(planck4(vx[i] + 273.15), 4) * vy[i]);
            x3y += (pow(planck4(vx[i] + 273.15), 3) * vy[i]);
            x2y += (pow(planck4(vx[i] + 273.15), 2) * vy[i]);
            xy += (planck4(vx[i] + 273.15) * vy[i]);
            y += vy[i];

        }

        // 拡大係数行列 M
        double M[8][8 + 1] =
        {
            {x14, x13, x12, x11, x10, x9, x8, x7, x7y},
            {x13, x12, x11, x10, x9, x8, x7, x6, x6y},
            {x12, x11, x10, x9, x8, x7, x6, x5, x5y},
            {x11, x10, x9, x8, x7, x6, x5, x4, x4y},
            {x10, x9, x8, x7, x6, x5, x4, x3, x3y},
            {x9, x8, x7, x6, x5, x4, x3, x2, x2y},
            {x8, x7, x6, x5, x4, x3, x2, x, xy},
            {x7, x6, x5, x4, x3, x2, x, double(vx.size()), y}};

        double **M2 = new double *[num];
        for (int i = 0; i < num; i++)
        {
            M2[i] = new double[num + 1];
        }

        for (int i = 0; i < num; i++)
        {
            for (int j = 0; j < num + 1; j++)
            {
                M2[i][j] = M[(8 - num) + i][(8 - num) + j];
            }
        }

        // 対角成分が1で正規化された階段行列を作る(前進消去)
        for (int i = 0; i < num; ++i)
        {
            // 対角成分の選択、この値で行成分を正規化
            pivot = M2[i][i];
            for (int j = 0; j < num + 1; ++j)
            {
                M2[i][j] = (1 / pivot) * M2[i][j];
            }

            // 階段行列を作る為に、現在の行より下の行について
            // i列目の成分が0になるような基本変形をする
            for (int k = i + 1; k < num; ++k)
            {
                mul = M2[k][i];
                for (int count = i; count < num + 1; ++count)
                {
                    M2[k][count] = M2[k][count] - mul * M2[i][count];
                }
            }
        }

        // 下から上に向かって変数に代入して、独立した解の形にする(後進代入)
        // このとき一番下の行はすでに独立した解を得ている
        for (int i = num - 1; i > 0; --i)
        {
            for (int k = i - 1; k >= 0; --k)
            {
                mul = M2[k][i];
                for (int count = i; count < num + 1; ++count)
                {
                    M2[k][count] = M2[k][count] - mul * M2[i][count];
                }
            }
        }

        //対応する係数を格納
        if (1 < num)
        {
            hh = M2[num - 1][num];
            gg = M2[num - 2][num];
        }
        if (2 < num)
        {
            ff = M2[num - 3][num];
        }
        if (3 < num)
        {
            ee = M2[num - 4][num];
        }
        if (4 < num)
        {
            dd = M2[num - 5][num];
        }
        if (5 < num)
        {
            cc = M2[num - 6][num];
        }
        if (6 < num)
        {
            bb = M2[num - 7][num];
        }
        if (7 < num)
        {
            aa = M2[num - 8][num];
        }

        //メモリの解放
        for (int i = 0; i < num; i++)
        {
            delete[] M2[i];
        }
        delete[] M2;

        //残差を求める
        double d = 0;
        double tmp;
        for (int i = 0; i < vx.size(); i++)
        {
            tmp = vy[i] - (gg * vx[i] + hh);
            if (tmp < 0)
            {
                d += tmp * -1;
            }
            else
            {
                d += tmp;
            }
        }
        d = d / vx.size();
        residual[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(d);
        arra[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(aa);
        arrb[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(bb);
        arrc[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(cc);
        arrd[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(dd);
        arre[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(ee);
        arrf[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(ff);
        arrg[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(gg);
        arrh[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(hh);

        saveFileInRegisterRegression("residual.csv",initialFileDirectory_thread);

        saveFileInRegisterRegression("a.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("b.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("c.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("d.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("e.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("f.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("g.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("h.csv",initialFileDirectory_thread);
        //ファイルに記述する際に使用する文を作成

        coefficient.append(QString::number(aa) + ",");
        coefficient.append(QString::number(bb) + ",");
        coefficient.append(QString::number(cc) + ",");
        coefficient.append(QString::number(dd) + ",");
        coefficient.append(QString::number(ee) + ",");
        coefficient.append(QString::number(ff) + ",");
        coefficient.append(QString::number(gg) + ",");
        coefficient.append(QString::number(hh) + ",");

        num = num - 1;

        if (2 == num)
        {
            formula.append(QString::number(ff) + "x^2 ");
        }
        else if (2 <= num)
        {
            if (ff < 0)
            {
                formula.append("- " + QString::number(-1 * ff) + "x^2 ");
            }
            else
            {
                formula.append("+ " + QString::number(ff) + "x^2 ");
            }
        }

        if (1 == num || num2_thread == "Black_Body")
        {
            if (num2_thread == "Black_Body")
                formula.append(QString::number(gg) + "*F( T + 273.15) ");
            else
                formula.append(QString::number(gg) + "x ");
        }
        else if (1 <= num)
        {
            if (gg < 0)
            {
                if (num2_thread == "Black_Body")
                    formula.append("- " + QString::number(-1 * gg) + "*F( T + 273.15) ");
                else
                    formula.append("- " + QString::number(-1 * gg) + "x ");
            }
            else
            {
                if (num2_thread == "Black_Body")
                    formula.append("+ " + QString::number(gg) + "*F( T + 273.15) ");
                else
                    formula.append("+ " + QString::number(gg) + "x ");
            }
        }

        if (hh < 0)
        {
            formula.append("- " + QString::number(-1 * hh));
        }
        else
        {
            formula.append("+ " + QString::number(hh));
        }

        s = QString::number(num) + "," + coefficient + formula;

        //取得した登録内容をファイルに書き込み
        QFile file(initialFileDirectory_thread + "/" + QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");
        vx.clear();
        vy.clear();
        tmp3.clear();
        //ファイルが存在する場合
        if (file.exists())
        {
            file.open(QIODevice::WriteOnly);
            QTextStream out(&file);

            out << s << endl;

            file.close();
        }
        else
        { //ファイルが存在しなかった場合
            //ファイル作成
            //QFile ini(QString::number(x-16) + "_" + QString::number(y-6) + "_" + tmp1[l]+ ".txt");
            QFile ini(QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");

            ini.open(QIODevice::WriteOnly);

            QTextStream out(&ini);

            out << s << endl;

            ini.close();

            tmp5.rename(QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt",
                        initialFileDirectory_thread + "/" + QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");
        }
        xycounter++;
        tmp3.clear();
    }

}

void ForThread4::run()
{

    int searchID, pairID;
    int xycounter = pixelxy_thread *0.5;
    QVector<QString> t;
    QVector<QVector<QString>> tmp3;
    QDir tmp5; //データパス用

    //   qDebug()<<tmp2_thread;
   // qDebug() << "thread4";

    for (int pix = pixelxy_thread *0.5; pix < pixelxy_thread*0.166*4; pix++)
    {
       
        //    qDebug()<<"pix"<<pix;
        for (int plotn = 0; plotn < Outputplotnumber_thread * 2; plotn++)
        {
            //qDebug()<<"<plotn"<<plotn;
            //  qDebug()<<tmp2[pixelxy*plotn+pix];//lをpixelxy回回すと同一ピクセルにもどる
            int currentpixel = pixelxy_thread * plotn + pix;
          //  qDebug()<<plotn;
            if (tmp2_thread[currentpixel][0].section('.', 2, 2) == "open"&& tmp2_thread[currentpixel][6]=="1")
            {
             //  qDebug()<<"def";
                searchID = tmp2_thread[currentpixel][3].toInt(0, 16);
                //IDが奇数のときの処理
                if (searchID % 2)
                {
                    pairID = searchID + 1;
                }
                else
                {
                    //IDが偶数の時の処理
                    pairID = searchID - 1;
                }
             //   qDebug()<<"ghk";
                //基本はopenの情報を格納しつつ、DN値のみ差分を格納
                for (int plotn2 = 0; plotn2 < Outputplotnumber_thread * 2; plotn2++)
                {
                    //     qDebug()<<"plotn2"<<plotn2;
                    int currentpixel2 = pixelxy_thread * plotn2 + pix;
                    if (tmp2_thread[currentpixel2][3].toInt(0, 16) == pairID && tmp2_thread[currentpixel][4] == tmp2_thread[currentpixel2][4] && tmp2_thread[currentpixel][5] == tmp2_thread[currentpixel2][5])
                    {
                        t.clear();
                        t.append(tmp2_thread[pixelxy_thread * plotn + pix][0]);
                        if (xAxis_thread == "diff DN")
                        {
                            t.append(QString::number(tmp2_thread[currentpixel][1].toDouble() - tmp2_thread[currentpixel2][1].toDouble()));
                            t.append(tmp2_thread[pixelxy_thread * plotn + pix][2]);
                        }
                        else if (yAxis_thread == "diff DN")
                        {
                            t.append(tmp2_thread[pixelxy_thread * plotn + pix][1]);
                            t.append(QString::number(tmp2_thread[currentpixel][2].toDouble() - tmp2_thread[currentpixel2][2].toDouble()));
                        }
                        tmp3.append(t);
                        break;
                    }
                }
            }                        //         qDebug()<<"mask0";
        }
//qDebug()<<"mask10513532";
        //  qDebug()<<tmp3;

        //ファイル作成処理
        //それぞれvx、vyに格納
        QVector<double> vx;
        QVector<double> vy;
        if (yAxis_thread == "diff DN" || xAxis_thread == "diff DN")
        {
            for (int l = 0; l < tmp3.size(); l++)
            {
                vx.append(tmp3[l][1].toDouble());
                vy.append(tmp3[l][2].toDouble());
            }
        }
        else
        {
            for (int l = 0; l < tmp2_thread.size(); l++)
            {
                vx.append(tmp2_thread[l][1].toDouble());
                vy.append(tmp2_thread[l][2].toDouble());
            }
        }

        cout<<"(";cout<<xaxis_thread[xycounter] - 16;cout<<",";cout<<yaxis_thread[xycounter] - 6;cout<<")";


        QString s;
        //回帰係数を計算し、登録の際に使用するSringを取得
        //   s = getRegressionCoefficientInRegisterRegressionforBlackbody(vx, vy, xaxis_thread[xycounter]-16, yaxis_thread[xycounter]-6);

        int num;
        double x = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0, x6 = 0, x7 = 0, x8 = 0, x9 = 0, x10 = 0, x11 = 0, x12 = 0, x13 = 0, x14 = 0;
        double x7y = 0, x6y = 0, x5y = 0, x4y = 0, x3y = 0, x2y = 0, xy = 0, y = 0;
        double pivot, mul;
        double aa, bb, cc, dd, ee, ff, gg, hh;
        QString coefficient, formula;
        //次数の取得 & +1の処理
        //num = ui->widget->selectedGraphs().at(0)->name().section(",",0,0).toInt() + 1;
        num = 2;
        //qDebug() << num;

        //係数の初期化
        aa = bb = cc = dd = ee = ff = gg = hh = 0;

        //それぞれの総和
        for (int i = 0; i < vx.size(); i++)
        {
            x += planck4(vx[i] + 273.15);
            x2 += pow(planck4(vx[i] + 273.15), 2);
            x3 += pow(planck4(vx[i] + 273.15), 3);
            x4 += pow(planck4(vx[i] + 273.15), 4);
            x5 += pow(planck4(vx[i] + 273.15), 5);
            x6 += pow(planck4(vx[i] + 273.15), 6);
            x7 += pow(planck4(vx[i] + 273.15), 7);
            x8 += pow(planck4(vx[i] + 273.15), 8);
            x9 += pow(planck4(vx[i] + 273.15), 9);
            x10 += pow(planck4(vx[i] + 273.15), 10);
            x11 += pow(planck4(vx[i] + 273.15), 11);
            x12 += pow(planck4(vx[i] + 273.15), 12);
            x13 += pow(planck4(vx[i] + 273.15), 13);
            x14 += pow(planck4(vx[i] + 273.15), 14);
            x7y += (pow(planck4(vx[i] + 273.15), 7) * vy[i]);
            x6y += (pow(planck4(vx[i] + 273.15), 6) * vy[i]);
            x5y += (pow(planck4(vx[i] + 273.15), 5) * vy[i]);
            x4y += (pow(planck4(vx[i] + 273.15), 4) * vy[i]);
            x3y += (pow(planck4(vx[i] + 273.15), 3) * vy[i]);
            x2y += (pow(planck4(vx[i] + 273.15), 2) * vy[i]);
            xy += (planck4(vx[i] + 273.15) * vy[i]);
            y += vy[i];

        }

        // 拡大係数行列 M
        double M[8][8 + 1] =
        {
            {x14, x13, x12, x11, x10, x9, x8, x7, x7y},
            {x13, x12, x11, x10, x9, x8, x7, x6, x6y},
            {x12, x11, x10, x9, x8, x7, x6, x5, x5y},
            {x11, x10, x9, x8, x7, x6, x5, x4, x4y},
            {x10, x9, x8, x7, x6, x5, x4, x3, x3y},
            {x9, x8, x7, x6, x5, x4, x3, x2, x2y},
            {x8, x7, x6, x5, x4, x3, x2, x, xy},
            {x7, x6, x5, x4, x3, x2, x, double(vx.size()), y}};

        double **M2 = new double *[num];
        for (int i = 0; i < num; i++)
        {
            M2[i] = new double[num + 1];
        }

        for (int i = 0; i < num; i++)
        {
            for (int j = 0; j < num + 1; j++)
            {
                M2[i][j] = M[(8 - num) + i][(8 - num) + j];
            }
        }

        // 対角成分が1で正規化された階段行列を作る(前進消去)
        for (int i = 0; i < num; ++i)
        {
            // 対角成分の選択、この値で行成分を正規化
            pivot = M2[i][i];
            for (int j = 0; j < num + 1; ++j)
            {
                M2[i][j] = (1 / pivot) * M2[i][j];
            }

            // 階段行列を作る為に、現在の行より下の行について
            // i列目の成分が0になるような基本変形をする
            for (int k = i + 1; k < num; ++k)
            {
                mul = M2[k][i];
                for (int count = i; count < num + 1; ++count)
                {
                    M2[k][count] = M2[k][count] - mul * M2[i][count];
                }
            }
        }

        // 下から上に向かって変数に代入して、独立した解の形にする(後進代入)
        // このとき一番下の行はすでに独立した解を得ている
        for (int i = num - 1; i > 0; --i)
        {
            for (int k = i - 1; k >= 0; --k)
            {
                mul = M2[k][i];
                for (int count = i; count < num + 1; ++count)
                {
                    M2[k][count] = M2[k][count] - mul * M2[i][count];
                }
            }
        }

        //対応する係数を格納
        if (1 < num)
        {
            hh = M2[num - 1][num];
            gg = M2[num - 2][num];
        }
        if (2 < num)
        {
            ff = M2[num - 3][num];
        }
        if (3 < num)
        {
            ee = M2[num - 4][num];
        }
        if (4 < num)
        {
            dd = M2[num - 5][num];
        }
        if (5 < num)
        {
            cc = M2[num - 6][num];
        }
        if (6 < num)
        {
            bb = M2[num - 7][num];
        }
        if (7 < num)
        {
            aa = M2[num - 8][num];
        }

        //メモリの解放
        for (int i = 0; i < num; i++)
        {
            delete[] M2[i];
        }
        delete[] M2;

        //残差を求める
        double d = 0;
        double tmp;
        for (int i = 0; i < vx.size(); i++)
        {
            tmp = vy[i] - (gg * vx[i] + hh);
            if (tmp < 0)
            {
                d += tmp * -1;
            }
            else
            {
                d += tmp;
            }
        }
        d = d / vx.size();
        residual[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(d);
        arra[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(aa);
        arrb[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(bb);
        arrc[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(cc);
        arrd[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(dd);
        arre[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(ee);
        arrf[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(ff);
        arrg[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(gg);
        arrh[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(hh);

        saveFileInRegisterRegression("residual.csv",initialFileDirectory_thread);

        saveFileInRegisterRegression("a.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("b.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("c.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("d.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("e.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("f.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("g.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("h.csv",initialFileDirectory_thread);
        //ファイルに記述する際に使用する文を作成

        coefficient.append(QString::number(aa) + ",");
        coefficient.append(QString::number(bb) + ",");
        coefficient.append(QString::number(cc) + ",");
        coefficient.append(QString::number(dd) + ",");
        coefficient.append(QString::number(ee) + ",");
        coefficient.append(QString::number(ff) + ",");
        coefficient.append(QString::number(gg) + ",");
        coefficient.append(QString::number(hh) + ",");

        num = num - 1;

        if (2 == num)
        {
            formula.append(QString::number(ff) + "x^2 ");
        }
        else if (2 <= num)
        {
            if (ff < 0)
            {
                formula.append("- " + QString::number(-1 * ff) + "x^2 ");
            }
            else
            {
                formula.append("+ " + QString::number(ff) + "x^2 ");
            }
        }

        if (1 == num || num2_thread == "Black_Body")
        {
            if (num2_thread == "Black_Body")
                formula.append(QString::number(gg) + "*F( T + 273.15) ");
            else
                formula.append(QString::number(gg) + "x ");
        }
        else if (1 <= num)
        {
            if (gg < 0)
            {
                if (num2_thread == "Black_Body")
                    formula.append("- " + QString::number(-1 * gg) + "*F( T + 273.15) ");
                else
                    formula.append("- " + QString::number(-1 * gg) + "x ");
            }
            else
            {
                if (num2_thread == "Black_Body")
                    formula.append("+ " + QString::number(gg) + "*F( T + 273.15) ");
                else
                    formula.append("+ " + QString::number(gg) + "x ");
            }
        }

        if (hh < 0)
        {
            formula.append("- " + QString::number(-1 * hh));
        }
        else
        {
            formula.append("+ " + QString::number(hh));
        }

        s = QString::number(num) + "," + coefficient + formula;

        //取得した登録内容をファイルに書き込み
        QFile file(initialFileDirectory_thread + "/" + QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");
        vx.clear();
        vy.clear();
        tmp3.clear();
        //ファイルが存在する場合
        if (file.exists())
        {
            file.open(QIODevice::WriteOnly);
            QTextStream out(&file);

            out << s << endl;

            file.close();
        }
        else
        { //ファイルが存在しなかった場合
            //ファイル作成
            //QFile ini(QString::number(x-16) + "_" + QString::number(y-6) + "_" + tmp1[l]+ ".txt");
            QFile ini(QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");

            ini.open(QIODevice::WriteOnly);

            QTextStream out(&ini);

            out << s << endl;

            ini.close();

            tmp5.rename(QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt",
                        initialFileDirectory_thread + "/" + QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");
        }
        xycounter++;
        tmp3.clear();
    }

}

void ForThread5::run()
{

    int searchID, pairID;
    int xycounter = pixelxy_thread *0.166*4;
    QVector<QString> t;
    QVector<QVector<QString>> tmp3;
    QDir tmp5; //データパス用

    //   qDebug()<<tmp2_thread;
   // qDebug() << "thread5";

    for (int pix = pixelxy_thread *0.166*4; pix < pixelxy_thread*0.166*5; pix++)
    {
    
        //    qDebug()<<"pix"<<pix;
        for (int plotn = 0; plotn < Outputplotnumber_thread * 2; plotn++)
        {
            //qDebug()<<"<plotn"<<plotn;
            //  qDebug()<<tmp2[pixelxy*plotn+pix];//lをpixelxy回回すと同一ピクセルにもどる
            int currentpixel = pixelxy_thread * plotn + pix;
          //  qDebug()<<plotn;
            if (tmp2_thread[currentpixel][0].section('.', 2, 2) == "open"&& tmp2_thread[currentpixel][6]=="1")
            {
             //   qDebug()<<"def";
                searchID = tmp2_thread[currentpixel][3].toInt(0, 16);
                //IDが奇数のときの処理
                if (searchID % 2)
                {
                    pairID = searchID + 1;
                }
                else
                {
                    //IDが偶数の時の処理
                    pairID = searchID - 1;
                }
               // qDebug()<<"ghk";
                //基本はopenの情報を格納しつつ、DN値のみ差分を格納
                for (int plotn2 = 0; plotn2 < Outputplotnumber_thread * 2; plotn2++)
                {
                    //     qDebug()<<"plotn2"<<plotn2;
                    int currentpixel2 = pixelxy_thread * plotn2 + pix;
                    if (tmp2_thread[currentpixel2][3].toInt(0, 16) == pairID && tmp2_thread[currentpixel][4] == tmp2_thread[currentpixel2][4] && tmp2_thread[currentpixel][5] == tmp2_thread[currentpixel2][5])
                    {
                        t.clear();
                        t.append(tmp2_thread[pixelxy_thread * plotn + pix][0]);
                        if (xAxis_thread == "diff DN")
                        {
                            t.append(QString::number(tmp2_thread[currentpixel][1].toDouble() - tmp2_thread[currentpixel2][1].toDouble()));
                            t.append(tmp2_thread[pixelxy_thread * plotn + pix][2]);
                        }
                        else if (yAxis_thread == "diff DN")
                        {
                            t.append(tmp2_thread[pixelxy_thread * plotn + pix][1]);
                            t.append(QString::number(tmp2_thread[currentpixel][2].toDouble() - tmp2_thread[currentpixel2][2].toDouble()));
                        }
                        tmp3.append(t);
                        break;
                    }
                }
            }    //                              qDebug()<<"mask0";
        }
//qDebug()<<"mask10513532";
        //  qDebug()<<tmp3;

        //ファイル作成処理
        //それぞれvx、vyに格納
        QVector<double> vx;
        QVector<double> vy;
        if (yAxis_thread == "diff DN" || xAxis_thread == "diff DN")
        {
            for (int l = 0; l < tmp3.size(); l++)
            {
                vx.append(tmp3[l][1].toDouble());
                vy.append(tmp3[l][2].toDouble());
            }
        }
        else
        {
            for (int l = 0; l < tmp2_thread.size(); l++)
            {
                vx.append(tmp2_thread[l][1].toDouble());
                vy.append(tmp2_thread[l][2].toDouble());
            }
        }

        cout<<"(";cout<<xaxis_thread[xycounter] - 16;cout<<",";cout<<yaxis_thread[xycounter] - 6;cout<<")";


        QString s;
        //回帰係数を計算し、登録の際に使用するSringを取得
        //   s = getRegressionCoefficientInRegisterRegressionforBlackbody(vx, vy, xaxis_thread[xycounter]-16, yaxis_thread[xycounter]-6);

        int num;
        double x = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0, x6 = 0, x7 = 0, x8 = 0, x9 = 0, x10 = 0, x11 = 0, x12 = 0, x13 = 0, x14 = 0;
        double x7y = 0, x6y = 0, x5y = 0, x4y = 0, x3y = 0, x2y = 0, xy = 0, y = 0;
        double pivot, mul;
        double aa, bb, cc, dd, ee, ff, gg, hh;
        QString coefficient, formula;
        //次数の取得 & +1の処理
        //num = ui->widget->selectedGraphs().at(0)->name().section(",",0,0).toInt() + 1;
        num = 2;
        //qDebug() << num;

        //係数の初期化
        aa = bb = cc = dd = ee = ff = gg = hh = 0;

        //それぞれの総和
        for (int i = 0; i < vx.size(); i++)
        {
            x += planck4(vx[i] + 273.15);
            x2 += pow(planck4(vx[i] + 273.15), 2);
            x3 += pow(planck4(vx[i] + 273.15), 3);
            x4 += pow(planck4(vx[i] + 273.15), 4);
            x5 += pow(planck4(vx[i] + 273.15), 5);
            x6 += pow(planck4(vx[i] + 273.15), 6);
            x7 += pow(planck4(vx[i] + 273.15), 7);
            x8 += pow(planck4(vx[i] + 273.15), 8);
            x9 += pow(planck4(vx[i] + 273.15), 9);
            x10 += pow(planck4(vx[i] + 273.15), 10);
            x11 += pow(planck4(vx[i] + 273.15), 11);
            x12 += pow(planck4(vx[i] + 273.15), 12);
            x13 += pow(planck4(vx[i] + 273.15), 13);
            x14 += pow(planck4(vx[i] + 273.15), 14);
            x7y += (pow(planck4(vx[i] + 273.15), 7) * vy[i]);
            x6y += (pow(planck4(vx[i] + 273.15), 6) * vy[i]);
            x5y += (pow(planck4(vx[i] + 273.15), 5) * vy[i]);
            x4y += (pow(planck4(vx[i] + 273.15), 4) * vy[i]);
            x3y += (pow(planck4(vx[i] + 273.15), 3) * vy[i]);
            x2y += (pow(planck4(vx[i] + 273.15), 2) * vy[i]);
            xy += (planck4(vx[i] + 273.15) * vy[i]);
            y += vy[i];

        }

        // 拡大係数行列 M
        double M[8][8 + 1] =
        {
            {x14, x13, x12, x11, x10, x9, x8, x7, x7y},
            {x13, x12, x11, x10, x9, x8, x7, x6, x6y},
            {x12, x11, x10, x9, x8, x7, x6, x5, x5y},
            {x11, x10, x9, x8, x7, x6, x5, x4, x4y},
            {x10, x9, x8, x7, x6, x5, x4, x3, x3y},
            {x9, x8, x7, x6, x5, x4, x3, x2, x2y},
            {x8, x7, x6, x5, x4, x3, x2, x, xy},
            {x7, x6, x5, x4, x3, x2, x, double(vx.size()), y}};

        double **M2 = new double *[num];
        for (int i = 0; i < num; i++)
        {
            M2[i] = new double[num + 1];
        }

        for (int i = 0; i < num; i++)
        {
            for (int j = 0; j < num + 1; j++)
            {
                M2[i][j] = M[(8 - num) + i][(8 - num) + j];
            }
        }

        // 対角成分が1で正規化された階段行列を作る(前進消去)
        for (int i = 0; i < num; ++i)
        {
            // 対角成分の選択、この値で行成分を正規化
            pivot = M2[i][i];
            for (int j = 0; j < num + 1; ++j)
            {
                M2[i][j] = (1 / pivot) * M2[i][j];
            }

            // 階段行列を作る為に、現在の行より下の行について
            // i列目の成分が0になるような基本変形をする
            for (int k = i + 1; k < num; ++k)
            {
                mul = M2[k][i];
                for (int count = i; count < num + 1; ++count)
                {
                    M2[k][count] = M2[k][count] - mul * M2[i][count];
                }
            }
        }

        // 下から上に向かって変数に代入して、独立した解の形にする(後進代入)
        // このとき一番下の行はすでに独立した解を得ている
        for (int i = num - 1; i > 0; --i)
        {
            for (int k = i - 1; k >= 0; --k)
            {
                mul = M2[k][i];
                for (int count = i; count < num + 1; ++count)
                {
                    M2[k][count] = M2[k][count] - mul * M2[i][count];
                }
            }
        }

        //対応する係数を格納
        if (1 < num)
        {
            hh = M2[num - 1][num];
            gg = M2[num - 2][num];
        }
        if (2 < num)
        {
            ff = M2[num - 3][num];
        }
        if (3 < num)
        {
            ee = M2[num - 4][num];
        }
        if (4 < num)
        {
            dd = M2[num - 5][num];
        }
        if (5 < num)
        {
            cc = M2[num - 6][num];
        }
        if (6 < num)
        {
            bb = M2[num - 7][num];
        }
        if (7 < num)
        {
            aa = M2[num - 8][num];
        }

        //メモリの解放
        for (int i = 0; i < num; i++)
        {
            delete[] M2[i];
        }
        delete[] M2;

        //残差を求める
        double d = 0;
        double tmp;
        for (int i = 0; i < vx.size(); i++)
        {
            tmp = vy[i] - (gg * vx[i] + hh);
            if (tmp < 0)
            {
                d += tmp * -1;
            }
            else
            {
                d += tmp;
            }
        }
        d = d / vx.size();
        residual[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(d);
        arra[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(aa);
        arrb[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(bb);
        arrc[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(cc);
        arrd[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(dd);
        arre[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(ee);
        arrf[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(ff);
        arrg[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(gg);
        arrh[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(hh);

        saveFileInRegisterRegression("residual.csv",initialFileDirectory_thread);

        saveFileInRegisterRegression("a.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("b.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("c.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("d.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("e.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("f.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("g.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("h.csv",initialFileDirectory_thread);
        //ファイルに記述する際に使用する文を作成

        coefficient.append(QString::number(aa) + ",");
        coefficient.append(QString::number(bb) + ",");
        coefficient.append(QString::number(cc) + ",");
        coefficient.append(QString::number(dd) + ",");
        coefficient.append(QString::number(ee) + ",");
        coefficient.append(QString::number(ff) + ",");
        coefficient.append(QString::number(gg) + ",");
        coefficient.append(QString::number(hh) + ",");

        num = num - 1;

        if (2 == num)
        {
            formula.append(QString::number(ff) + "x^2 ");
        }
        else if (2 <= num)
        {
            if (ff < 0)
            {
                formula.append("- " + QString::number(-1 * ff) + "x^2 ");
            }
            else
            {
                formula.append("+ " + QString::number(ff) + "x^2 ");
            }
        }

        if (1 == num || num2_thread == "Black_Body")
        {
            if (num2_thread == "Black_Body")
                formula.append(QString::number(gg) + "*F( T + 273.15) ");
            else
                formula.append(QString::number(gg) + "x ");
        }
        else if (1 <= num)
        {
            if (gg < 0)
            {
                if (num2_thread == "Black_Body")
                    formula.append("- " + QString::number(-1 * gg) + "*F( T + 273.15) ");
                else
                    formula.append("- " + QString::number(-1 * gg) + "x ");
            }
            else
            {
                if (num2_thread == "Black_Body")
                    formula.append("+ " + QString::number(gg) + "*F( T + 273.15) ");
                else
                    formula.append("+ " + QString::number(gg) + "x ");
            }
        }

        if (hh < 0)
        {
            formula.append("- " + QString::number(-1 * hh));
        }
        else
        {
            formula.append("+ " + QString::number(hh));
        }

        s = QString::number(num) + "," + coefficient + formula;

        //取得した登録内容をファイルに書き込み
        QFile file(initialFileDirectory_thread + "/" + QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");
        vx.clear();
        vy.clear();
        tmp3.clear();
        //ファイルが存在する場合
        if (file.exists())
        {
            file.open(QIODevice::WriteOnly);
            QTextStream out(&file);

            out << s << endl;

            file.close();
        }
        else
        { //ファイルが存在しなかった場合
            //ファイル作成
            //QFile ini(QString::number(x-16) + "_" + QString::number(y-6) + "_" + tmp1[l]+ ".txt");
            QFile ini(QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");

            ini.open(QIODevice::WriteOnly);

            QTextStream out(&ini);

            out << s << endl;

            ini.close();

            tmp5.rename(QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt",
                        initialFileDirectory_thread + "/" + QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");
        }
        xycounter++;
        tmp3.clear();
    }

}

void ForThread6::run()
{

    int searchID, pairID;
    int xycounter = pixelxy_thread *0.166*5;
    QVector<QString> t;
    QVector<QVector<QString>> tmp3;
    QDir tmp5; //データパス用

    //   qDebug()<<tmp2_thread;
    qDebug() << "thread6";
    for (int pix = pixelxy_thread *0.166*5; pix < pixelxy_thread; pix++)
    {
    
        //    qDebug()<<"pix"<<pix;
        for (int plotn = 0; plotn < Outputplotnumber_thread * 2; plotn++)
        {
            //qDebug()<<"<plotn"<<plotn;
            //  qDebug()<<tmp2[pixelxy*plotn+pix];//lをpixelxy回回すと同一ピクセルにもどる
            int currentpixel = pixelxy_thread * plotn + pix;

           // qDebug()<<plotn;
            if (tmp2_thread[currentpixel][0].section('.', 2, 2) == "open"&& tmp2_thread[currentpixel][6]=="1")
            {
             //  qDebug()<<"def";
                searchID = tmp2_thread[currentpixel][3].toInt(0, 16);
                //IDが奇数のときの処理
                if (searchID % 2)
                {
                    pairID = searchID + 1;
                }
                else
                {
                    //IDが偶数の時の処理
                    pairID = searchID - 1;
                }
               // qDebug()<<"ghk";
                //基本はopenの情報を格納しつつ、DN値のみ差分を格納
                for (int plotn2 = 0; plotn2 < Outputplotnumber_thread * 2; plotn2++)
                {
                    //     qDebug()<<"plotn2"<<plotn2;
                    int currentpixel2 = pixelxy_thread * plotn2 + pix;
                    if (tmp2_thread[currentpixel2][3].toInt(0, 16) == pairID && tmp2_thread[currentpixel][4] == tmp2_thread[currentpixel2][4] && tmp2_thread[currentpixel][5] == tmp2_thread[currentpixel2][5])
                    {
                        t.clear();
                        t.append(tmp2_thread[pixelxy_thread * plotn + pix][0]);
                        if (xAxis_thread == "diff DN")
                        {
                            t.append(QString::number(tmp2_thread[currentpixel][1].toDouble() - tmp2_thread[currentpixel2][1].toDouble()));
                            t.append(tmp2_thread[pixelxy_thread * plotn + pix][2]);
                        }
                        else if (yAxis_thread == "diff DN")
                        {
                            t.append(tmp2_thread[pixelxy_thread * plotn + pix][1]);
                            t.append(QString::number(tmp2_thread[currentpixel][2].toDouble() - tmp2_thread[currentpixel2][2].toDouble()));
                        }
                        tmp3.append(t);
                        break;
                    }
                }
            }              //                    qDebug()<<"mask0";
        }
//qDebug()<<"mask10513532";
        //  qDebug()<<tmp3;

        //ファイル作成処理
        //それぞれvx、vyに格納
        QVector<double> vx;
        QVector<double> vy;
        if (yAxis_thread == "diff DN" || xAxis_thread == "diff DN")
        {
            for (int l = 0; l < tmp3.size(); l++)
            {
                vx.append(tmp3[l][1].toDouble());
                vy.append(tmp3[l][2].toDouble());
            }
        }
        else
        {
            for (int l = 0; l < tmp2_thread.size(); l++)
            {
                vx.append(tmp2_thread[l][1].toDouble());
                vy.append(tmp2_thread[l][2].toDouble());
            }
        }

        cout<<"(";cout<<xaxis_thread[xycounter] - 16;cout<<",";cout<<yaxis_thread[xycounter] - 6;cout<<")";


        QString s;
        //回帰係数を計算し、登録の際に使用するSringを取得
        //   s = getRegressionCoefficientInRegisterRegressionforBlackbody(vx, vy, xaxis_thread[xycounter]-16, yaxis_thread[xycounter]-6);

        int num;
        double x = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0, x6 = 0, x7 = 0, x8 = 0, x9 = 0, x10 = 0, x11 = 0, x12 = 0, x13 = 0, x14 = 0;
        double x7y = 0, x6y = 0, x5y = 0, x4y = 0, x3y = 0, x2y = 0, xy = 0, y = 0;
        double pivot, mul;
        double aa, bb, cc, dd, ee, ff, gg, hh;
        QString coefficient, formula;
        //次数の取得 & +1の処理
        //num = ui->widget->selectedGraphs().at(0)->name().section(",",0,0).toInt() + 1;
        num = 2;
        //qDebug() << num;

        //係数の初期化
        aa = bb = cc = dd = ee = ff = gg = hh = 0;

        //それぞれの総和
        for (int i = 0; i < vx.size(); i++)
        {
            x += planck4(vx[i] + 273.15);
            x2 += pow(planck4(vx[i] + 273.15), 2);
            x3 += pow(planck4(vx[i] + 273.15), 3);
            x4 += pow(planck4(vx[i] + 273.15), 4);
            x5 += pow(planck4(vx[i] + 273.15), 5);
            x6 += pow(planck4(vx[i] + 273.15), 6);
            x7 += pow(planck4(vx[i] + 273.15), 7);
            x8 += pow(planck4(vx[i] + 273.15), 8);
            x9 += pow(planck4(vx[i] + 273.15), 9);
            x10 += pow(planck4(vx[i] + 273.15), 10);
            x11 += pow(planck4(vx[i] + 273.15), 11);
            x12 += pow(planck4(vx[i] + 273.15), 12);
            x13 += pow(planck4(vx[i] + 273.15), 13);
            x14 += pow(planck4(vx[i] + 273.15), 14);
            x7y += (pow(planck4(vx[i] + 273.15), 7) * vy[i]);
            x6y += (pow(planck4(vx[i] + 273.15), 6) * vy[i]);
            x5y += (pow(planck4(vx[i] + 273.15), 5) * vy[i]);
            x4y += (pow(planck4(vx[i] + 273.15), 4) * vy[i]);
            x3y += (pow(planck4(vx[i] + 273.15), 3) * vy[i]);
            x2y += (pow(planck4(vx[i] + 273.15), 2) * vy[i]);
            xy += (planck4(vx[i] + 273.15) * vy[i]);
            y += vy[i];

        }

        // 拡大係数行列 M
        double M[8][8 + 1] =
        {
            {x14, x13, x12, x11, x10, x9, x8, x7, x7y},
            {x13, x12, x11, x10, x9, x8, x7, x6, x6y},
            {x12, x11, x10, x9, x8, x7, x6, x5, x5y},
            {x11, x10, x9, x8, x7, x6, x5, x4, x4y},
            {x10, x9, x8, x7, x6, x5, x4, x3, x3y},
            {x9, x8, x7, x6, x5, x4, x3, x2, x2y},
            {x8, x7, x6, x5, x4, x3, x2, x, xy},
            {x7, x6, x5, x4, x3, x2, x, double(vx.size()), y}};

        double **M2 = new double *[num];
        for (int i = 0; i < num; i++)
        {
            M2[i] = new double[num + 1];
        }

        for (int i = 0; i < num; i++)
        {
            for (int j = 0; j < num + 1; j++)
            {
                M2[i][j] = M[(8 - num) + i][(8 - num) + j];
            }
        }

        // 対角成分が1で正規化された階段行列を作る(前進消去)
        for (int i = 0; i < num; ++i)
        {
            // 対角成分の選択、この値で行成分を正規化
            pivot = M2[i][i];
            for (int j = 0; j < num + 1; ++j)
            {
                M2[i][j] = (1 / pivot) * M2[i][j];
            }

            // 階段行列を作る為に、現在の行より下の行について
            // i列目の成分が0になるような基本変形をする
            for (int k = i + 1; k < num; ++k)
            {
                mul = M2[k][i];
                for (int count = i; count < num + 1; ++count)
                {
                    M2[k][count] = M2[k][count] - mul * M2[i][count];
                }
            }
        }

        // 下から上に向かって変数に代入して、独立した解の形にする(後進代入)
        // このとき一番下の行はすでに独立した解を得ている
        for (int i = num - 1; i > 0; --i)
        {
            for (int k = i - 1; k >= 0; --k)
            {
                mul = M2[k][i];
                for (int count = i; count < num + 1; ++count)
                {
                    M2[k][count] = M2[k][count] - mul * M2[i][count];
                }
            }
        }

        //対応する係数を格納
        if (1 < num)
        {
            hh = M2[num - 1][num];
            gg = M2[num - 2][num];
        }
        if (2 < num)
        {
            ff = M2[num - 3][num];
        }
        if (3 < num)
        {
            ee = M2[num - 4][num];
        }
        if (4 < num)
        {
            dd = M2[num - 5][num];
        }
        if (5 < num)
        {
            cc = M2[num - 6][num];
        }
        if (6 < num)
        {
            bb = M2[num - 7][num];
        }
        if (7 < num)
        {
            aa = M2[num - 8][num];
        }

        //メモリの解放
        for (int i = 0; i < num; i++)
        {
            delete[] M2[i];
        }
        delete[] M2;

        //残差を求める
        double d = 0;
        double tmp;
        for (int i = 0; i < vx.size(); i++)
        {
            tmp = vy[i] - (gg * vx[i] + hh);
            if (tmp < 0)
            {
                d += tmp * -1;
            }
            else
            {
                d += tmp;
            }
        }
        d = d / vx.size();
        residual[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(d);
        arra[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(aa);
        arrb[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(bb);
        arrc[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(cc);
        arrd[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(dd);
        arre[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(ee);
        arrf[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(ff);
        arrg[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(gg);
        arrh[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(hh);

        saveFileInRegisterRegression("residual.csv",initialFileDirectory_thread);

        saveFileInRegisterRegression("a.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("b.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("c.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("d.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("e.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("f.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("g.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("h.csv",initialFileDirectory_thread);
        //ファイルに記述する際に使用する文を作成

        coefficient.append(QString::number(aa) + ",");
        coefficient.append(QString::number(bb) + ",");
        coefficient.append(QString::number(cc) + ",");
        coefficient.append(QString::number(dd) + ",");
        coefficient.append(QString::number(ee) + ",");
        coefficient.append(QString::number(ff) + ",");
        coefficient.append(QString::number(gg) + ",");
        coefficient.append(QString::number(hh) + ",");

        num = num - 1;

        if (2 == num)
        {
            formula.append(QString::number(ff) + "x^2 ");
        }
        else if (2 <= num)
        {
            if (ff < 0)
            {
                formula.append("- " + QString::number(-1 * ff) + "x^2 ");
            }
            else
            {
                formula.append("+ " + QString::number(ff) + "x^2 ");
            }
        }

        if (1 == num || num2_thread == "Black_Body")
        {
            if (num2_thread == "Black_Body")
                formula.append(QString::number(gg) + "*F( T + 273.15) ");
            else
                formula.append(QString::number(gg) + "x ");
        }
        else if (1 <= num)
        {
            if (gg < 0)
            {
                if (num2_thread == "Black_Body")
                    formula.append("- " + QString::number(-1 * gg) + "*F( T + 273.15) ");
                else
                    formula.append("- " + QString::number(-1 * gg) + "x ");
            }
            else
            {
                if (num2_thread == "Black_Body")
                    formula.append("+ " + QString::number(gg) + "*F( T + 273.15) ");
                else
                    formula.append("+ " + QString::number(gg) + "x ");
            }
        }

        if (hh < 0)
        {
            formula.append("- " + QString::number(-1 * hh));
        }
        else
        {
            formula.append("+ " + QString::number(hh));
        }

        s = QString::number(num) + "," + coefficient + formula;

        //取得した登録内容をファイルに書き込み
        QFile file(initialFileDirectory_thread + "/" + QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");
        vx.clear();
        vy.clear();
        tmp3.clear();
        //ファイルが存在する場合
        if (file.exists())
        {
            file.open(QIODevice::WriteOnly);
            QTextStream out(&file);

            out << s << endl;

            file.close();
        }
        else
        { //ファイルが存在しなかった場合
            //ファイル作成
            //QFile ini(QString::number(x-16) + "_" + QString::number(y-6) + "_" + tmp1[l]+ ".txt");
            QFile ini(QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");

            ini.open(QIODevice::WriteOnly);

            QTextStream out(&ini);

            out << s << endl;

            ini.close();

            tmp5.rename(QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt",
                        initialFileDirectory_thread + "/" + QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");
        }
        xycounter++;
        tmp3.clear();
    }

}

CalibrationGraph::CalibrationGraph(QWidget *parent) : QDialog(parent),
    ui(new Ui::CalibrationGraph)
{
    
    ui->setupUi(this);

    //グラフウィジェットの初期設定
    ui->widget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->widget->setInteraction(QCP::iSelectPlottables);

    connect(ui->widget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequest(QPoint)));
    connect(ui->widget, SIGNAL(plottableClick(QCPAbstractPlottable *, QMouseEvent *)), this, SLOT(graphClicked(QCPAbstractPlottable *)));
    connect(ui->widget, SIGNAL(mousePress(QMouseEvent *)), this, SLOT(mousePress()));

    filterpath.cd("../../../");

    //荒井式の温度計算に必要な変数
    h_planck = 6.62606957e-34;
    kB = 1.3806488e-23;
    c_speed = 299792458;
    c1 = 2 * M_PI * h_planck * pow(c_speed, 2);
    c2 = (h_planck * c_speed) / kB;
    SIGMA = 5.670373e-8;

    Outputplotnumberini = 0;
    Outputplotnumber = 0;
    numfortxt = 0;

    loadFilter();
}

CalibrationGraph::~CalibrationGraph()
{
    delete ui;
}

void CalibrationGraph::popCalibrationGraph(QVector<QVector<QString>> p, QString xAxisName, QString yAxisName, QString lineType, int itemNum, bool ismodified)
{
    //10000回以上のグラフの呼び出しがされた場合、Hidden状態のものを再利用
    //もし10000個のグラフが同時表示されていた場合は0番目から順次変更して表示

    ismodifiedgl = ismodified;

    if (sum < 10000)
    {
        cg[n] = new CalibrationGraph;
    }
    else
    {
        for (int i = 0; i < 10000; i++)
        {
            if (cg[i]->isHidden())
            {
                n = i;
            }
        }
        if (n == 10000)
        {
            n = 0;
        }
    }

    //初期化
    cg[n]->vx.clear();
    cg[n]->vy.clear();
    cg[n]->pixelList.clear();
    cg[n]->pixelList2.clear();
    cg[n]->diff.clear();
    cg[n]->replot.clear();
    cg[n]->regressionGraphCount = 1;
    cg[n]->axisFlag = false;
    cg[n]->infoNum = 0;
    cg[n]->previousNum = 0;
    cg[n]->plotNum = 0;
    cg[n]->regressionGraphCount = 1;

    //もし中身がない場合、関数を終了
    if (p[0][0] == "empty")
    {
        return;
    }

    //Activeに変更(今のところ使い道なし)
    isActive[n] = 1;

    //データの総数を記憶
    cg[n]->previousNum = itemNum;

    //ピクセル情報を格納
    cg[n]->pixelList = p;
    cg[n]->replot = p;

    //x軸、y軸の名前を格納
    cg[n]->xAxis = xAxisName;
    cg[n]->yAxis = yAxisName;

    //各係数の最大値と最小値の取得
    setMax();
    setMin();

    //x座標、y座標をそれぞれ格納(今のところ使い道なし)
    xPosition[n] = cg[n]->pixelList[0][1].toInt();
    yPosition[n] = cg[n]->pixelList[0][2].toInt();

    //window titleを設定
    cg[n]->setWindowTitle("Position : (" + QString::number(xPosition[n]) + "," + QString::number(yPosition[n]) + ")" + "   X : " + xAxisName + "   Y : " + yAxisName);

    //軸の名前を元に各種処理をする
    int f = judgeAxis(xAxisName, yAxisName);

    if (f == -1)
    { //xにもyにもDN関連が入っていない時
        cg[n]->vx = getAxisValue(xAxisName, cg[n]->pixelList, itemNum);
        cg[n]->vy = getAxisValue(yAxisName, cg[n]->pixelList, itemNum);
    }
    else if (f == 0)
    { //xもyもDN関連が入っている時
        cg[n]->vx = getAxisValue2(xAxisName, cg[n]->pixelList, itemNum);
        cg[n]->vy = getAxisValue2(yAxisName, cg[n]->pixelList, itemNum);
    }
    else if (f == 1)
    { //xだけDN関連が入っている時
        cg[n]->vx = getAxisValue2(xAxisName, cg[n]->pixelList, itemNum);
        cg[n]->vy = getAxisValue(yAxisName, pixelList2, infoNum);
    }
    else if (f == 2)
    { //yだけDN関連が入っている時
        cg[n]->vy = getAxisValue2(yAxisName, cg[n]->pixelList, itemNum);
        cg[n]->vx = getAxisValue(xAxisName, pixelList2, infoNum);
    }

    //open close diff等の情報を格納
    cg[n]->pixelList2 = pixelList2;

    //プロットした数を記憶
    cg[n]->infoNum = infoNum;
    cg[n]->plotNum = infoNum;

    //プロットした数配列に格納
    Outputplotnumberini = cg[n]->plotNum;

    //プロットしている数のセット
    cg[n]->ui->plotNumberBrowser->clear();
    if (f == -1)
    {
        cg[n]->ui->plotNumberBrowser->setText(QString::number(itemNum) + " / " + QString::number(itemNum));
    }
    else
    {
        cg[n]->ui->plotNumberBrowser->setText(QString::number(infoNum) + " / " + QString::number(infoNum));
    }

    //diff関連のリプロットに時間がかかるため対策
    //diff関連のグラフの場合はdiffとreplotに最初からdiffDNの情報を格納
    if ((f == 1 || f == 2) && (xAxisName == "diff DN" || yAxisName == "diff DN"))
    {
        cg[n]->diff = pixelList2;
        cg[n]->replot = pixelList2;
    }

    setInitializeUI(); //uiの設定

    //グラフの設定
    cg[n]->ui->widget->addGraph(); //0番目のグラフを追加
    cg[n]->drawGraph(cg[n]->vx, cg[n]->vy, lineType); //グラフの描画

    cg[n]->move(300, 50);    //表示場所の設定
    cg[n]->show();           //表示
    cg[n]->raise();          //最前列に表示
    cg[n]->activateWindow(); //アクティブウィンドウにする

    sum++;
    n++;
}

void CalibrationGraph::setInitializeUI()
{

    //初期化
    cg[n]->ui->tgtMaxLineEdit->clear();
    cg[n]->ui->tgtMinLineEdit->clear();
    cg[n]->ui->boloMaxLineEdit->clear();
    cg[n]->ui->boloMinLineEdit->clear();
    cg[n]->ui->pkgMaxLineEdit->clear();
    cg[n]->ui->pkgMinLineEdit->clear();
    cg[n]->ui->caseMaxLineEdit->clear();
    cg[n]->ui->caseMinLineEdit->clear();
    cg[n]->ui->shMaxLineEdit->clear();
    cg[n]->ui->shMinLineEdit->clear();
    cg[n]->ui->lensMaxLineEdit->clear();
    cg[n]->ui->lensMinLineEdit->clear();

    cg[n]->ui->regressionFormulaBrowser->clear();
    /*
    cg[n]->ui->widget->clearGraphs();
    cg[n]->ui->coefficient_a->clear();
    cg[n]->ui->coefficient_b->clear();
    cg[n]->ui->coefficient_c->clear();
    cg[n]->ui->coefficient_d->clear();
    cg[n]->ui->coefficient_e->clear();
    cg[n]->ui->coefficient_f->clear();
    cg[n]->ui->coefficient_g->clear();
    cg[n]->ui->coefficient_h->clear();
    */
    //係数にダブル型だけが入るように制限
    QDoubleValidator *v = new QDoubleValidator(this);
    /*
    cg[n]->ui->coefficient_a->setValidator(v);
    cg[n]->ui->coefficient_b->setValidator(v);
    cg[n]->ui->coefficient_c->setValidator(v);
    cg[n]->ui->coefficient_d->setValidator(v);
    cg[n]->ui->coefficient_e->setValidator(v);
    cg[n]->ui->coefficient_f->setValidator(v);
    cg[n]->ui->coefficient_g->setValidator(v);
    cg[n]->ui->coefficient_h->setValidator(v);
    //スライダー範囲の設定
*/
    cg[n]->ui->tgtMaxSlider->setRange(tgtMin, tgtMax);
    cg[n]->ui->tgtMinSlider->setRange(tgtMin, tgtMax);

    cg[n]->ui->boloMaxSlider->setRange(boloMin, boloMax);
    cg[n]->ui->boloMinSlider->setRange(boloMin, boloMax);

    cg[n]->ui->pkgMaxSlider->setRange(pkgMin, pkgMax);
    cg[n]->ui->pkgMinSlider->setRange(pkgMin, pkgMax);

    cg[n]->ui->caseMaxSlider->setRange(caseMin, caseMax);
    cg[n]->ui->caseMinSlider->setRange(caseMin, caseMax);

    cg[n]->ui->shMaxSlider->setRange(shMin, shMax);
    cg[n]->ui->shMinSlider->setRange(shMin, shMax);

    cg[n]->ui->lensMaxSlider->setRange(lensMin, lensMax);
    cg[n]->ui->lensMinSlider->setRange(lensMin, lensMax);

    //スライダー目盛りのセット

    cg[n]->ui->tgtMaxSlider->setTickInterval((tgtMax - tgtMin) / 10);
    cg[n]->ui->tgtMinSlider->setTickInterval((tgtMax - tgtMin) / 10);

    cg[n]->ui->boloMaxSlider->setTickInterval((boloMax - boloMin) / 10);
    cg[n]->ui->boloMinSlider->setTickInterval((boloMax - boloMin) / 10);

    cg[n]->ui->pkgMaxSlider->setTickInterval((pkgMax - pkgMin) / 10);
    cg[n]->ui->pkgMinSlider->setTickInterval((pkgMax - pkgMin) / 10);

    cg[n]->ui->caseMaxSlider->setTickInterval((caseMax - caseMin) / 10);
    cg[n]->ui->caseMinSlider->setTickInterval((caseMax - caseMin) / 10);

    cg[n]->ui->shMaxSlider->setTickInterval((shMax - shMin) / 10);
    cg[n]->ui->shMinSlider->setTickInterval((shMax - shMin) / 10);

    cg[n]->ui->lensMaxSlider->setTickInterval((lensMax - lensMin) / 10);
    cg[n]->ui->lensMinSlider->setTickInterval((lensMax - lensMin) / 10);

    //スライダー初期位置の設定
    cg[n]->ui->tgtMaxSlider->setValue(tgtMax);
    cg[n]->ui->tgtMinSlider->setValue(tgtMin);

    cg[n]->ui->boloMaxSlider->setValue(boloMax);
    cg[n]->ui->boloMinSlider->setValue(boloMin);

    cg[n]->ui->pkgMaxSlider->setValue(0);
    cg[n]->ui->pkgMinSlider->setValue(0);

    cg[n]->ui->caseMaxSlider->setValue(0);
    cg[n]->ui->caseMinSlider->setValue(0);

    cg[n]->ui->shMaxSlider->setValue(0);
    cg[n]->ui->shMinSlider->setValue(0);

    cg[n]->ui->lensMaxSlider->setValue(0);
    cg[n]->ui->lensMinSlider->setValue(0);

    /*
    //スライダーの値をテキストエディットに格納
    cg[n]->ui->tgtMaxLineEdit->setText(QString::number((double)cg[n]->ui->tgtMaxSlider->value() / 1000));
    cg[n]->ui->tgtMinLineEdit->setText(QString::number((double)cg[n]->ui->tgtMinSlider->value() /1000));

    cg[n]->ui->boloMaxLineEdit->setText(QString::number((double)cg[n]->ui->boloMaxSlider->value() / 1000));
    cg[n]->ui->boloMinLineEdit->setText(QString::number((double)cg[n]->ui->boloMinSlider->value() / 1000));

    cg[n]->ui->pkgMaxLineEdit->setText(QString::number((double)cg[n]->ui->pkgMaxSlider->value() / 1000));
    cg[n]->ui->pkgMinLineEdit->setText(QString::number((double)cg[n]->ui->pkgMinSlider->value() / 1000));

    cg[n]->ui->caseMaxLineEdit->setText(QString::number((double)cg[n]->ui->caseMaxSlider->value() / 1000));
    cg[n]->ui->caseMinLineEdit->setText(QString::number((double)cg[n]->ui->caseMinSlider->value() / 1000));

    cg[n]->ui->shMaxLineEdit->setText(QString::number((double)cg[n]->ui->shMaxSlider->value() / 1000));
    cg[n]->ui->shMinLineEdit->setText(QString::number((double)cg[n]->ui->shMinSlider->value() / 1000));

    cg[n]->ui->lensMaxLineEdit->setText(QString::number((double)cg[n]->ui->lensMaxSlider->value() / 1000));
    cg[n]->ui->lensMinLineEdit->setText(QString::number((double)cg[n]->ui->lensMinSlider->value() / 1000));
*/
}

//x軸、y軸にopen DN、close DN、diff DNが入っているかの確認
int CalibrationGraph::judgeAxis(QString x, QString y)
{
    if (x == "open DN" || x == "close DN" || x == "diff DN")
    {
        if (y == "open DN" || y == "close DN" || y == "diff DN")
        {
            return 0; //xもyもDN関連が入っている
        }
        return 1; //xだけDN関連が入っている
    }

    if (y == "open DN" || y == "close DN" || y == "diff DN")
    {
        if (x == "open DN" || x == "close DN" || x == "diff DN")
        {
            return 0; //xもyもDN関連が入っている
        }
        return 2; //yだけDN関連が入っている
    }

    return -1; //xにもyにもDN関連が入っていない
}

//グラフ入力する値を個別に格納
QVector<double> CalibrationGraph::getAxisValue(QString axis, QVector<QVector<QString>> info, int num)
{

    QVector<double> v(num);

    if (axis == "No.")
    {
        for (int i = 0; i < num; i++)
        {
            v[i] = i + 1;
        }
        return v;
    }
    else if (axis == "pkg T(degC)")
    {
        for (int i = 0; i < num; i++)
        {
            v[i] = info[i][5].toDouble();
        }
        return v;
    }
    else if (axis == "case T(degC)")
    {
        for (int i = 0; i < num; i++)
        {
            v[i] = info[i][6].toDouble();
        }
        return v;
    }
    else if (axis == "shtr T(degC)")
    {
        for (int i = 0; i < num; i++)
        {
            v[i] = info[i][7].toDouble();
        }
        return v;
    }
    else if (axis == "lens T(degC)")
    {
        for (int i = 0; i < num; i++)
        {
            v[i] = info[i][8].toDouble();
        }
        return v;
    }
    else if (axis == "open DN" || axis == "close DN" || axis == "diff DN")
    {
        for (int i = 0; i < num; i++)
        {
            v[i] = info[i][3].toDouble();
            /* cout<<"diff DN ";
            cout<<info[i][3].toDouble()<<endl;

*/}
        return v;
    }
    else if (axis == "target T(degC)")
    {
        for (int i = 0; i < num; i++)
        {
            v[i] = info[i][9].toDouble();
        }
        return v;
    }
    return v;
}

//軸にopen DN、close DN、diff DNが入ってる場合の前準備
QVector<double> CalibrationGraph::getAxisValue2(QString axis, QVector<QVector<QString>> info, int itemNum)
{

    infoNum = 0; //open close diffの数用
    pixelList2.clear();

    //open close diff、それぞれの情報をpixelList2に格納
    if (axis == "open DN")
    {
        //open画像の情報を格納
        QVector<QString> tmp1;
        infoNum = 0;
        for (int i = 0; i < itemNum; i++)
        {
            if (info[i][0].section('.', 2, 2) == "open")
            {
                tmp1.clear();
                for (int j = 0; j < 25; j++)
                {
                    tmp1.append(info[i][j]);
                }

                pixelList2.append(tmp1);
                infoNum++;
            }
        }

        //そのまま.openだけのグラフを表示
        return getAxisValue(axis, pixelList2, infoNum);
    }
    else if (axis == "close DN")
    {
        //close画像の情報を格納
        QVector<QString> tmp1;
        infoNum = 0;
        for (int i = 0; i < itemNum; i++)
        {
            if (info[i][0].section('.', 2, 2) == "close")
            {
                tmp1.clear();
                for (int j = 0; j < 25; j++)
                {
                    tmp1.append(info[i][j]);
                }
                pixelList2.append(tmp1);
                infoNum++;
            }
        }
        //そのまま.closeだけのグラフを表示
        return getAxisValue(axis, pixelList2, infoNum);
    }
    else
    { //diffDNの場合

        //プログレス用のダイアログ
        QProgressDialog p;
        p.setLabelText("Search Pair Progress");
        p.setRange(0, itemNum);
        p.setCancelButton(0);

        //ペアを探す処理&diffに情報を格納しつつ、openとcloseの差をpixelList2に格納
        QVector<QString> tmp1;
        int searchID, pairID;

        for (int i = 0; i < itemNum; i++)
        {
          /* qDebug()<<"oppai";
            qDebug()<<info[1][0];
            qDebug()<<info[1][1];
            qDebug()<<info[1][2];
            qDebug()<<info[1][3];
            qDebug()<<info[1][4];
            qDebug()<<info[1][5];
            qDebug()<<info[1][6];
            qDebug()<<info[1][7];
            qDebug()<<info[1][8];
            qDebug()<<info[1][9];
            qDebug()<<info[1][10];
            qDebug()<<info[1][11];
            qDebug()<<info[1][12];
*/
        }


        infoNum = 0;
        for (int i = 0; i < itemNum; i++)
        {
            if (info[i][0].section('.', 2, 2) == "open")
            {
                //16進数の文字列を10進数に変換
                searchID = info[i][10].toInt(0, 16);

                //IDが奇数のときの処理
                if (searchID % 2)
                {
                    pairID = searchID + 1;
                }
                else
                {
                    //IDが偶数の時の処理
                    pairID = searchID - 1;
                }

                //基本はopenの情報を格納しつつ、DN値のみ差分を格納
                for (int j = 0; j < itemNum; j++)
                {
                    if (info[j][10].toInt(0, 16) == pairID && info[i][11] == info[j][11] && info[i][12] == info[j][12])
                    {
                        if(((info[i][3].toDouble() - info[j][3].toDouble()<150)&&(info[i][11].toStdString()=="BB")&&info[i][9].toInt()==50)||
                                ((info[i][3].toDouble() - info[j][3].toDouble()<150)&&(info[i][11].toStdString()=="Oil_bath_BB")&&info[i][9].toInt()==50)||
                                ((info[i][3].toDouble() - info[j][3].toDouble()<325)&&(info[i][11].toStdString()=="Oil_bath_BB")&&info[i][9].toInt()==75)||
                                ((info[i][3].toDouble() - info[j][3].toDouble()<550)&&(info[i][11].toStdString()=="Oil_bath_BB")&&info[i][9].toInt()==100)||
                                ((info[i][3].toDouble() - info[j][3].toDouble()<750)&&(info[i][11].toStdString()=="Oil_bath_BB")&&info[i][9].toInt()==125)){
                        }


                        else{
                            tmp1.clear();
                            for (int m = 0; m < 25; m++)
                            {
                                //m=3の時はDN値の差分を格納、他はそのまま格納
                                if (m == 3)
                                {
                                    tmp1.append(QString::number(info[i][3].toDouble() - info[j][3].toDouble()));
                                    if (info[i][3].toDouble() - info[j][3].toDouble() > 1000)
                                    {
                                        /*
                                        qDebug() << info[i][0];
                                        qDebug() << info[i][1];
                                        qDebug() << info[i][2];
                                        qDebug() << info[i][3];
                                        qDebug() << info[j][0];
                                        qDebug() << info[j][1];
                                        qDebug() << info[j][2];
                                        qDebug() << info[j][3];
                                        qDebug() << info[i][3].toDouble() - info[j][3].toDouble();
  */
                                    }

                                    //  cout<<info[i][3].toDouble() - info[j][3].toDouble()<<endl;

                                }
                                else
                                {
                                    tmp1.append(info[i][m]);
                                }
                            }

                            pixelList2.append(tmp1);
                            infoNum++;
                            break;
                        }
                    }
                }
            }
            //プログレス表示
            p.setValue(i);
            p.show();
            QCoreApplication::processEvents();
        }

        //そのまま、格納した情報でグラフを表示
        return getAxisValue(axis, pixelList2, infoNum);
    }
}

//グラフの描画
void CalibrationGraph::drawGraph(QVector<double> x, QVector<double> y, QString lineType)
{

    //初期化
    ui->widget->graph(0)->clearData();

    ui->widget->xAxis->setLabel(xAxis); //横軸の名前
    ui->widget->yAxis->setLabel(yAxis); //縦軸の名前

    //グラフの線の種類の設定 & 点の大きさの設定
    if (lineType == "solid line")
    {
        ui->widget->graph(0)->setLineStyle(QCPGraph::lsLine);
    }
    else
    {
        ui->widget->graph(0)->setLineStyle(QCPGraph::lsNone);
    }
    ui->widget->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 2));

    ui->widget->graph(0)->setData(x, y); //グラフデータをセット
    ui->widget->graph(0)->setPen(QPen(Qt::black));
    ui->widget->rescaleAxes(true); // 軸の描画

    //x軸とy軸の最大値、最小値を取得
    xMax = *std::max_element(x.constBegin(), x.constEnd());
    xMin = *std::min_element(x.constBegin(), x.constEnd());
    yMax = *std::max_element(y.constBegin(), y.constEnd());
    yMin = *std::min_element(y.constBegin(), y.constEnd());

    //軸の最大値、最小値の設定
    //軸の設定は一番最初にグラフを描画した時のみ
    if (axisFlag == false)
    {
        xRangeMin = xMin - ((xMax - xMin) / 10);
        xRangeMax = xMax + ((xMax - xMin) / 10);
        yRangeMin = yMin - ((yMax - yMin) / 10);
        yRangeMax = yMax + ((yMax - yMin) / 10);

        axisFlag = true;
    }

    //軸の範囲を設定
    ui->widget->xAxis->setRange(xRangeMin, xRangeMax);
    ui->widget->yAxis->setRange(yRangeMin, yRangeMax);

    ui->widget->replot(); //グラフ描画
}


//回帰ボタンが押された時の挙動
void CalibrationGraph::on_regressionButton_clicked()
{

    int size = 10000, num = ui->degreeComboBox->currentText().toInt();
    double min = xRangeMin;
    QVector<double> x(size), y(size);
    QString formula = "y = ";
    QString coefficient;
    QString num2 = ui->degreeComboBox->currentText();

    //グラフに異常がある場合は、関数を終了
    if (vx.size() < 1 || vy.size() < 1)
    {
        ui->regressionFormulaBrowser->setText("The graph is not appropriate.");
        return;
    }

    //回帰係数を求める
    if (num2 == "Black_Body")
        setRegressionCoefficientforBlack(vx, vy);
    else
        setRegressionCoefficient(vx, vy);
    //回帰式とグラフネームを設定
    if (7 <= num)
    {
        formula.append(QString::number(a) + "x^7 ");
    }

    coefficient.append(QString::number(a) + ",");
    coefficient.append(QString::number(b) + ",");
    coefficient.append(QString::number(c) + ",");
    coefficient.append(QString::number(d) + ",");
    coefficient.append(QString::number(e) + ",");
    coefficient.append(QString::number(f) + ",");
    coefficient.append(QString::number(g) + ",");
    coefficient.append(QString::number(h) + ",");

    if (6 == num)
    {
        formula.append(QString::number(b) + "x^6 ");
    }
    else if (6 <= num)
    {
        if (b < 0)
        {
            formula.append("- " + QString::number(-1 * b) + "x^6 ");
        }
        else
        {
            formula.append("+ " + QString::number(b) + "x^6 ");
        }
    }

    if (5 == num)
    {
        formula.append(QString::number(c) + "x^5 ");
    }
    else if (5 <= num)
    {
        if (c < 0)
        {
            formula.append("- " + QString::number(-1 * c) + "x^5 ");
        }
        else
        {
            formula.append("+ " + QString::number(c) + "x^5 ");
        }
    }

    if (4 == num)
    {
        formula.append(QString::number(d) + "x^4 ");
    }
    else if (4 <= num)
    {
        if (d < 0)
        {
            formula.append("- " + QString::number(-1 * d) + "x^4 ");
        }
        else
        {
            formula.append("+ " + QString::number(d) + "x^4 ");
        }
    }

    if (3 == num)
    {
        formula.append(QString::number(e) + "x^3 ");
    }
    else if (3 <= num)
    {
        if (e < 0)
        {
            formula.append("- " + QString::number(-1 * e) + "x^3 ");
        }
        else
        {
            formula.append("+ " + QString::number(e) + "x^3 ");
        }
    }

    if (2 == num)
    {
        formula.append(QString::number(f) + "x^2 ");
    }
    else if (2 <= num)
    {
        if (f < 0)
        {
            formula.append("- " + QString::number(-1 * f) + "x^2 ");
        }
        else
        {
            formula.append("+ " + QString::number(f) + "x^2 ");
        }
    }

    if (1 == num || num2 == "Black_Body")
    {
        if (num2 == "Black_Body")
            formula.append(QString::number(g) + "*F( T + 273.15) ");
        else
            formula.append(QString::number(g) + "x ");
    }
    else if (1 <= num)
    {
        if (g < 0)
        {
            if (num2 == "Black_Body")
                formula.append("- " + QString::number(-1 * g) + "*F( T + 273.15) ");
            else
                formula.append("- " + QString::number(-1 * g) + "x ");
        }
        else
        {
            if (num2 == "Black_Body")
                formula.append("+ " + QString::number(g) + "*F( T + 273.15) ");
            else
                formula.append("+ " + QString::number(g) + "x ");
        }
    }
    if (h < 0)
    {
        formula.append("- " + QString::number(-1 * h));
    }
    else
    {
        formula.append("+ " + QString::number(h));
    }

    //回帰式を描画するためにxとyに値を代入
    if (num2 == "Black_Body")
    {
        for (int i = 0; i < size; i++)
        {
            x[i] = min;
            y[i] = g * planck(min + 273.15) + h;
            min += (xRangeMax - xRangeMin) / 10000;
        }
    }
    else
    {
        for (int i = 0; i < size; i++)
        {
            x[i] = min;
            y[i] = a * pow(min, 7) + b * pow(min, 6) + c * pow(min, 5) + d * pow(min, 4) + e * pow(min, 3) + f * pow(min, 2) + g * min + h;
            min += (xRangeMax - xRangeMin) / 10000;
        }
    }

    //ペアを探す処理
    QVector<QString> tmp1;
    int searchID, pairID;

    if ((xAxis == "diff DN") || (yAxis == "diff DN"))
    {
        for (int i = 0; i < replot.size(); i++)
        {
            //16進数の文字列を10進数に変換
            searchID = replot[i][10].toInt(0, 16);

            //IDが奇数のときの処理
            if (searchID % 2)
            {
                pairID = searchID + 1;
            }
            else
            {
                //IDが偶数の時の処理
                pairID = searchID - 1;
            }

            //pixelListの中からファイル名が一致したピクセルのファイル名を格納
            for (int j = 0; j < pixelList.size(); j++)
            {
                if (pixelList[j][0] == replot[i][0])
                {
                    tmp1.append(pixelList[j][0]);
                    break;
                }
            }

            //ペアとなるピクセルのファイル名を格納
            for (int j = 0; j < pixelList.size(); j++)
            {
                if (pixelList[j][10].toInt(0, 16) == pairID && replot[i][11] == pixelList[j][11] && replot[i][12] == pixelList[j][12])
                {
                    tmp1.append(pixelList[j][0]);
                    break;
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < replot.size(); i++)
        {
            if (xAxis == "open DN" || yAxis == "open DN")
            {
                if (replot[i][0].section('.', 2, 2) == "open")
                {
                    tmp1.append(replot[i][0]);
                }
            }
            else if (yAxis == "close DN" || xAxis == "close DN")
            {
                if (replot[i][0].section('.', 2, 2) == "close")
                {
                    tmp1.append(replot[i][0]);
                }
            }
            else
            {
                tmp1.append(replot[i][0]);
            }
        }
    }

    //絞り込まれたファイル名を取得
    QString searchName;
    searchName.clear();
    //  numfortxt=tmp1.size();

    for (int l = 0; l < tmp1.size(); l++)
    {
        searchName.append("tirimageinfo.img_file='" + tmp1[l] + "'");
        if (l + 1 != tmp1.size())
        {
            searchName.append(" or ");
        }
    }

    //qDebug() << searchName;
    ui->widget->addGraph(); //回帰用のグラフの追加
    ui->widget->graph(regressionGraphCount)->setData(x, y); //グラフデータをセット
    ui->widget->graph(regressionGraphCount)->setPen(QPen(Qt::red)); //色の設定
    ui->widget->graph(regressionGraphCount)->setName(QString::number(num) + "," + coefficient + formula + "," + searchName); //グラフの名前の設定
    ui->widget->replot(); //再描画

    regressionGraphCount++;
}

//回帰係数を求める関数
void CalibrationGraph::setRegressionCoefficient(QVector<double> vx, QVector<double> vy)
{

    int num;
    double x = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0, x6 = 0, x7 = 0, x8 = 0, x9 = 0, x10 = 0, x11 = 0, x12 = 0, x13 = 0, x14 = 0;
    double x7y = 0, x6y = 0, x5y = 0, x4y = 0, x3y = 0, x2y = 0, xy = 0, y = 0;
    double pivot, mul;

    //次数の取得 & +1の処理
    num = ui->degreeComboBox->currentText().toInt() + 1;

    //係数の初期化
    a = b = c = d = e = f = g = h = 0;

    //それぞれの総和
    for (int i = 0; i < vx.size(); i++)
    {
        x += vx[i];
        x2 += pow(vx[i], 2);
        x3 += pow(vx[i], 3);
        x4 += pow(vx[i], 4);
        x5 += pow(vx[i], 5);
        x6 += pow(vx[i], 6);
        x7 += pow(vx[i], 7);
        x8 += pow(vx[i], 8);
        x9 += pow(vx[i], 9);
        x10 += pow(vx[i], 10);
        x11 += pow(vx[i], 11);
        x12 += pow(vx[i], 12);
        x13 += pow(vx[i], 13);
        x14 += pow(vx[i], 14);
        x7y += (pow(vx[i], 7) * vy[i]);
        x6y += (pow(vx[i], 6) * vy[i]);
        x5y += (pow(vx[i], 5) * vy[i]);
        x4y += (pow(vx[i], 4) * vy[i]);
        x3y += (pow(vx[i], 3) * vy[i]);
        x2y += (pow(vx[i], 2) * vy[i]);
        xy += (vx[i] * vy[i]);
        y += vy[i];
    }

    // 拡大係数行列 M
    double M[8][8 + 1] =
    {
        {x14, x13, x12, x11, x10, x9, x8, x7, x7y},
        {x13, x12, x11, x10, x9, x8, x7, x6, x6y},
        {x12, x11, x10, x9, x8, x7, x6, x5, x5y},
        {x11, x10, x9, x8, x7, x6, x5, x4, x4y},
        {x10, x9, x8, x7, x6, x5, x4, x3, x3y},
        {x9, x8, x7, x6, x5, x4, x3, x2, x2y},
        {x8, x7, x6, x5, x4, x3, x2, x, xy},
        {x7, x6, x5, x4, x3, x2, x, double(vx.size()), y}};

    double **M2 = new double *[num];
    for (int i = 0; i < num; i++)
    {
        M2[i] = new double[num + 1];
    }

    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < num + 1; j++)
        {
            M2[i][j] = M[(8 - num) + i][(8 - num) + j];
        }
    }

    // 対角成分が1で正規化された階段行列を作る(前進消去)
    for (int i = 0; i < num; ++i)
    {
        // 対角成分の選択、この値で行成分を正規化
        pivot = M2[i][i];
        for (int j = 0; j < num + 1; ++j)
        {
            M2[i][j] = (1 / pivot) * M2[i][j];
        }

        // 階段行列を作る為に、現在の行より下の行について
        // i列目の成分が0になるような基本変形をする
        for (int k = i + 1; k < num; ++k)
        {
            mul = M2[k][i];
            for (int count = i; count < num + 1; ++count)
            {
                M2[k][count] = M2[k][count] - mul * M2[i][count];
            }
        }
    }

    // 下から上に向かって変数に代入して、独立した解の形にする(後進代入)
    // このとき一番下の行はすでに独立した解を得ている
    for (int i = num - 1; i > 0; --i)
    {
        for (int k = i - 1; k >= 0; --k)
        {
            mul = M2[k][i];
            for (int count = i; count < num + 1; ++count)
            {
                M2[k][count] = M2[k][count] - mul * M2[i][count];
            }
        }
    }

    //対応する係数を格納
    if (1 < num)
    {
        h = M2[num - 1][num];
        g = M2[num - 2][num];
    }
    if (2 < num)
    {
        f = M2[num - 3][num];
    }
    if (3 < num)
    {
        e = M2[num - 4][num];
    }
    if (4 < num)
    {
        d = M2[num - 5][num];
    }
    if (5 < num)
    {
        c = M2[num - 6][num];
    }
    if (6 < num)
    {
        b = M2[num - 7][num];
    }
    if (7 < num)
    {
        a = M2[num - 8][num];
    }

    //メモリの解放
    for (int i = 0; i < num; i++)
    {
        delete[] M2[i];
    }
    delete[] M2;
}

void CalibrationGraph::setRegressionCoefficientforBlack(QVector<double> vx, QVector<double> vy)
{

    int num;
    double x = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0, x6 = 0, x7 = 0, x8 = 0, x9 = 0, x10 = 0, x11 = 0, x12 = 0, x13 = 0, x14 = 0;
    double x7y = 0, x6y = 0, x5y = 0, x4y = 0, x3y = 0, x2y = 0, xy = 0, y = 0;
    double pivot, mul;

    //次数の取得 & +1の処理
    num = 2;

    //係数の初期化
    a = b = c = d = e = f = g = h = 0;

    //それぞれの総和
    for (int i = 0; i < vx.size(); i++)
    {
        x += planck(vx[i] + 273.15);
        x2 += pow(planck(vx[i] + 273.15), 2);
        x3 += pow(planck(vx[i] + 273.15), 3);
        x4 += pow(planck(vx[i] + 273.15), 4);
        x5 += pow(planck(vx[i] + 273.15), 5);
        x6 += pow(planck(vx[i] + 273.15), 6);
        x7 += pow(planck(vx[i] + 273.15), 7);
        x8 += pow(planck(vx[i] + 273.15), 8);
        x9 += pow(planck(vx[i] + 273.15), 9);
        x10 += pow(planck(vx[i] + 273.15), 10);
        x11 += pow(planck(vx[i] + 273.15), 11);
        x12 += pow(planck(vx[i] + 273.15), 12);
        x13 += pow(planck(vx[i] + 273.15), 13);
        x14 += pow(planck(vx[i] + 273.15), 14);
        x7y += (pow(planck(vx[i] + 273.15), 7) * vy[i]);
        x6y += (pow(planck(vx[i] + 273.15), 6) * vy[i]);
        x5y += (pow(planck(vx[i] + 273.15), 5) * vy[i]);
        x4y += (pow(planck(vx[i] + 273.15), 4) * vy[i]);
        x3y += (pow(planck(vx[i] + 273.15), 3) * vy[i]);
        x2y += (pow(planck(vx[i] + 273.15), 2) * vy[i]);
        xy += (planck(vx[i] + 273.15) * vy[i]);
        y += vy[i];
    }

    // 拡大係数行列 M
    double M[8][8 + 1] =
    {
        {x14, x13, x12, x11, x10, x9, x8, x7, x7y},
        {x13, x12, x11, x10, x9, x8, x7, x6, x6y},
        {x12, x11, x10, x9, x8, x7, x6, x5, x5y},
        {x11, x10, x9, x8, x7, x6, x5, x4, x4y},
        {x10, x9, x8, x7, x6, x5, x4, x3, x3y},
        {x9, x8, x7, x6, x5, x4, x3, x2, x2y},
        {x8, x7, x6, x5, x4, x3, x2, x, xy},
        {x7, x6, x5, x4, x3, x2, x, double(vx.size()), y}};

    double **M2 = new double *[num];
    for (int i = 0; i < num; i++)
    {
        M2[i] = new double[num + 1];
    }

    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < num + 1; j++)
        {
            M2[i][j] = M[(8 - num) + i][(8 - num) + j];
        }
    }

    // 対角成分が1で正規化された階段行列を作る(前進消去)
    for (int i = 0; i < num; ++i)
    {
        // 対角成分の選択、この値で行成分を正規化
        pivot = M2[i][i];
        for (int j = 0; j < num + 1; ++j)
        {
            M2[i][j] = (1 / pivot) * M2[i][j];
        }

        // 階段行列を作る為に、現在の行より下の行について
        // i列目の成分が0になるような基本変形をする
        for (int k = i + 1; k < num; ++k)
        {
            mul = M2[k][i];
            for (int count = i; count < num + 1; ++count)
            {
                M2[k][count] = M2[k][count] - mul * M2[i][count];
            }
        }
    }

    // 下から上に向かって変数に代入して、独立した解の形にする(後進代入)
    // このとき一番下の行はすでに独立した解を得ている
    for (int i = num - 1; i > 0; --i)
    {
        for (int k = i - 1; k >= 0; --k)
        {
            mul = M2[k][i];
            for (int count = i; count < num + 1; ++count)
            {
                M2[k][count] = M2[k][count] - mul * M2[i][count];
            }
        }
    }

    //対応する係数を格納
    if (1 < num)
    {
        h = M2[num - 1][num];
        g = M2[num - 2][num];
    }
    if (2 < num)
    {
        f = M2[num - 3][num];
    }
    if (3 < num)
    {
        e = M2[num - 4][num];
    }
    if (4 < num)
    {
        d = M2[num - 5][num];
    }
    if (5 < num)
    {
        c = M2[num - 6][num];
    }
    if (6 < num)
    {
        b = M2[num - 7][num];
    }
    if (7 < num)
    {
        a = M2[num - 8][num];
    }

    //メモリの解放
    for (int i = 0; i < num; i++)
    {
        delete[] M2[i];
    }
    delete[] M2;
}

//左上のバツボタンで閉じられた際の処理
void CalibrationGraph::closeEvent(QCloseEvent *e)
{

    //一応、メモリの解放 & 初期化
    vx.clear();
    vy.clear();
    pixelList.clear();
    pixelList2.clear();
    diff.clear();
    replot.clear();
    regressionGraphCount = 1;
    axisFlag = false;
    infoNum = 0;
    previousNum = 0;
    plotNum = 0;
    regressionGraphCount = 1;

    ui->widget->clearGraphs();

#ifndef QT_NO_WHATSTHIS
    if (isModal() && QWhatsThis::inWhatsThisMode())
        QWhatsThis::leaveWhatsThisMode();
#endif
    if (isVisible())
    {
        QPointer<QObject> that = this;
        reject();
        if (that && isVisible())
            e->ignore();
    }
    else
    {
        e->accept();
    }
}

//各パラメータの最大値を設定
void CalibrationGraph::setMax()
{

    tgtMax = boloMax = pkgMax = caseMax = shMax = lensMax = 0;

    for (int i = 0; i < cg[n]->previousNum; i++)
    {
        if (boloMax < cg[n]->pixelList[i][4].toDouble() * 1000)
        {
            boloMax = cg[n]->pixelList[i][4].toDouble() * 1000;
        }
        if (pkgMax < cg[n]->pixelList[i][5].toDouble() * 1000)
        {
            pkgMax = cg[n]->pixelList[i][5].toDouble() * 1000;
        }
        if (caseMax < cg[n]->pixelList[i][6].toDouble() * 1000)
        {
            caseMax = cg[n]->pixelList[i][6].toDouble() * 1000;
        }
        if (shMax < cg[n]->pixelList[i][7].toDouble() * 1000)
        {
            shMax = cg[n]->pixelList[i][7].toDouble() * 1000;
        }
        if (lensMax < cg[n]->pixelList[i][8].toDouble() * 1000)
        {
            lensMax = cg[n]->pixelList[i][8].toDouble() * 1000;
        }
        if (tgtMax < cg[n]->pixelList[i][9].toDouble() * 1000)
        {
            tgtMax = cg[n]->pixelList[i][9].toDouble() * 1000;
        }
    }
}

//各パラメータの最小値を設定
void CalibrationGraph::setMin()
{

    tgtMin = boloMin = pkgMin = caseMin = shMin = lensMin = 10000 * 1000;

    for (int i = 0; i < cg[n]->previousNum; i++)
    {
        if (boloMin > cg[n]->pixelList[i][4].toDouble() * 1000)
        {
            boloMin = cg[n]->pixelList[i][4].toDouble() * 1000;
        }
        if (pkgMin > cg[n]->pixelList[i][5].toDouble() * 1000)
        {
            pkgMin = cg[n]->pixelList[i][5].toDouble() * 1000;
        }
        if (caseMin > cg[n]->pixelList[i][6].toDouble() * 1000)
        {
            caseMin = cg[n]->pixelList[i][6].toDouble() * 1000;
        }
        if (shMin > cg[n]->pixelList[i][7].toDouble() * 1000)
        {
            shMin = cg[n]->pixelList[i][7].toDouble() * 1000;
        }
        if (lensMin > cg[n]->pixelList[i][8].toDouble() * 1000)
        {
            lensMin = cg[n]->pixelList[i][8].toDouble() * 1000;
        }
        if (tgtMin > cg[n]->pixelList[i][9].toDouble() * 1000)
        {
            tgtMin = cg[n]->pixelList[i][9].toDouble() * 1000;
        }
    }
}

//リプロットボタンを押された時の挙動
void CalibrationGraph::on_replotButton_clicked()
{
    double tgt_max, tgt_min, bolo_max, bolo_min, pkg_max, pkg_min;
    double case_max, case_min, sh_max, sh_min, lens_max, lens_min;
    int count = 0;

    //vx、vyを初期化
    vx.clear();
    vy.clear();

    //入力されている範囲を取得
    tgt_max = ui->tgtMaxLineEdit->text().toDouble();
    tgt_min = ui->tgtMinLineEdit->text().toDouble();

    bolo_max = ui->boloMaxLineEdit->text().toDouble();
    bolo_min = ui->boloMinLineEdit->text().toDouble();

    pkg_max = ui->pkgMaxLineEdit->text().toDouble();
    pkg_min = ui->pkgMinLineEdit->text().toDouble();

    case_max = ui->caseMaxLineEdit->text().toDouble();
    case_min = ui->caseMinLineEdit->text().toDouble();

    sh_max = ui->shMaxLineEdit->text().toDouble();
    sh_min = ui->shMinLineEdit->text().toDouble();

    lens_max = ui->lensMaxLineEdit->text().toDouble();
    lens_min = ui->lensMinLineEdit->text().toDouble();

    QVector<QString> tmp;
    replot.clear();

    //diffの時はサーチペアを何回もしなようにdiff専用の処理をする
    int f = judgeAxis(xAxis, yAxis);

    if ((f == 1 || f == 2) && (xAxis == "diff DN" || yAxis == "diff DN"))
    {
        //tmpに条件が合致したピクセルの情報を格納
        count = 0;
        for (int i = 0; i < infoNum; i++)
        {
            if (tgt_min <= diff[i][9].toDouble() && diff[i][9].toDouble() <= tgt_max)
            {
                if (bolo_min <= diff[i][4].toDouble() && diff[i][4].toDouble() <= bolo_max)
                {
                    if (pkg_min <= diff[i][5].toDouble() && diff[i][5].toDouble() <= pkg_max)
                    {
                        if (case_min <= diff[i][6].toDouble() && diff[i][6].toDouble() <= case_max)
                        {
                            if (sh_min <= diff[i][7].toDouble() && diff[i][7].toDouble() <= sh_max)
                            {
                                if (lens_min <= diff[i][8].toDouble() && diff[i][8].toDouble() <= lens_max)
                                {
                                    tmp.clear();
                                    for (int j = 0; j < 25; j++)
                                    {
                                        tmp.append(diff[i][j]);
                                    }
                                    replot.append(tmp);
                                    count++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        //tmpに条件が合致したピクセルの情報を格納
        count = 0;
        for (int i = 0; i < previousNum; i++)
        {
            if (tgt_min <= pixelList[i][9].toDouble() && pixelList[i][9].toDouble() <= tgt_max)
            {
                if (bolo_min <= pixelList[i][4].toDouble() && pixelList[i][4].toDouble() <= bolo_max)
                {
                    if (pkg_min <= pixelList[i][5].toDouble() && pixelList[i][5].toDouble() <= pkg_max)
                    {
                        if (case_min <= pixelList[i][6].toDouble() && pixelList[i][6].toDouble() <= case_max)
                        {
                            if (sh_min <= pixelList[i][7].toDouble() && pixelList[i][7].toDouble() <= sh_max)
                            {
                                if (lens_min <= pixelList[i][8].toDouble() && pixelList[i][8].toDouble() <= lens_max)
                                {
                                    tmp.clear();
                                    for (int j = 0; j < 25; j++)
                                    {
                                        tmp.append(pixelList[i][j]);
                                    }
                                    replot.append(tmp);
                                    count++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //軸の名前を元に各種処理をする
    if (f == -1)
    {
        vx = getAxisValue(xAxis, replot, count);
        vy = getAxisValue(yAxis, replot, count);
    }
    else if (f == 0)
    {
        vx = getAxisValue2(xAxis, replot, count);
        vy = getAxisValue2(yAxis, replot, count);
    }
    else if (f == 1)
    {
        if (xAxis == "diff DN")
        {
            vx = getAxisValue(xAxis, replot, count);
            vy = getAxisValue(yAxis, replot, count);
        }
        else
        {
            vx = getAxisValue2(xAxis, replot, count);
            vy = getAxisValue(yAxis, pixelList2, infoNum);
        }
    }
    else if (f == 2)
    {
        if (yAxis == "diff DN")
        {
            vy = getAxisValue(yAxis, replot, count);
            vx = getAxisValue(xAxis, replot, count);
        }
        else
        {
            vy = getAxisValue2(yAxis, replot, count);
            vx = getAxisValue(xAxis, pixelList2, infoNum);
        }
    }

    //プロットしている数のセット
    ui->plotNumberBrowser->clear();
    if (f == -1)
    {
        ui->plotNumberBrowser->setText(QString::number(count) + " / " + QString::number(previousNum));
    }
    else if (xAxis == "diff DN" || yAxis == "diff DN")
    {
        ui->plotNumberBrowser->setText(QString::number(count) + " / " + QString::number(plotNum));
    }
    else
    {
        ui->plotNumberBrowser->setText(QString::number(infoNum) + " / " + QString::number(plotNum));
    }
    //cout<<"plotnum(絞り込み後)";
    //cout<<count<<endl;
    Outputplotnumber = count;

    //グラフの描画
    drawGraph(vx, vy, "");
}

//csvファイル出力が押された時の挙動
void CalibrationGraph::on_outputCSVFileButton_clicked()
{
    //ファイル名を取得
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save CSV"), "", tr("CSV (*.csv);;TXT (*.txt)"));

    //ファイル名が空白なら終了
    if (fileName == "")
    {
        return;
    }

    QFile file(fileName);

    //ファイルが開けなっかた場合終了
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::information(this, tr("Unable to open file"), file.errorString());
        return;
    }

    QTextStream out(&file);

    //一番上のカラム行
    out << "img_file" << ',' << "x" << ',' << "y" << ',' << "pixel" << ',';
    out << "mask" << ',' << "thumbnail" << ',' << "path" << ',' << "date_time" << ',';
    out << "m" << ',' << "place" << ',' << "target_name" << ',' << "phi" << ',';
    out << "hood_t" << ',' << "target_t" << ',' << "len_t" << ',' << "plt_t" << ',';
    out << "bol_t" << ',' << "img_id" << ',' << "plt_t_set" << ',' << "bol_t_mon" << ',';
    out << "pkg_t_mon" << ',' << "case_t_mon" << ',' << "sh_t_mon" << ',' << "lens_t_mon" << endl;

    int f = judgeAxis(xAxis, yAxis);

    //diffでなければリプロット時に格納した情報を出力、diffの場合はペアを探してからペアと合わせて出力
    if (f == -1 || f == 0)
    {
        for (int i = 0; i < replot.size(); i++)
        {
            out << replot[i][0] << ',' << replot[i][1] << ',' << replot[i][2] << ',' << replot[i][3] << ',';
            out << replot[i][13] << ',' << replot[i][15] << ',' << replot[i][12] << ',' << replot[i][16] << ',';
            out << replot[i][17] << ',' << replot[i][18] << ',' << replot[i][11] << ',' << replot[i][19] << ',';
            out << replot[i][20] << ',' << replot[i][9] << ',' << replot[i][21] << ',' << replot[i][22] << ',';
            out << replot[i][23] << ',' << replot[i][10] << ',' << replot[i][24] << ',' << replot[i][4] << ',';
            out << replot[i][5] << ',' << replot[i][6] << ',' << replot[i][7] << ',' << replot[i][8] << endl;
        }
    }
    else
    {
        //軸にdiff DNがある場合
        if (xAxis == "diff DN" || yAxis == "diff DN")
        {
            //プログレスダイアログの設定
            QProgressDialog p;
            p.setLabelText("Search Pair Progress");
            p.setRange(0, replot.size());
            p.setCancelButton(0);

            //ペアを探す処理&diffに情報を格納しつつ、openとcloseの差をdiffに格納
            QVector<QString> tmp1;
            QVector<QVector<QString>> tmp2;
            int searchID, pairID;

            for (int i = 0; i < replot.size(); i++)
            {
                //16進数の文字列を10進数に変換
                searchID = replot[i][10].toInt(0, 16);

                //IDが奇数のときの処理
                if (searchID % 2)
                {
                    pairID = searchID + 1;
                }
                else
                {
                    //IDが偶数の時の処理
                    pairID = searchID - 1;
                }

                //pixelListの中から名前が一致したピクセルの情報を格納
                for (int j = 0; j < pixelList.size(); j++)
                {
                    if (pixelList[j][0] == replot[i][0])
                    {
                        tmp1.clear();
                        for (int m = 0; m < 25; m++)
                        {
                            tmp1.append(pixelList[j][m]);
                        }

                        tmp2.append(tmp1);
                        break;
                    }
                }

                //ペアとなるピクセルの情報を格納
                for (int j = 0; j < pixelList.size(); j++)
                {
                    if (pixelList[j][10].toInt(0, 16) == pairID && replot[i][11] == pixelList[j][11] && replot[i][12] == pixelList[j][12])
                    {
                        tmp1.clear();
                        for (int m = 0; m < 25; m++)
                        {
                            tmp1.append(pixelList[j][m]);
                        }

                        tmp2.append(tmp1);
                        break;
                    }
                }

                //プログレス表示
                p.setValue(i);
                p.show();
                QCoreApplication::processEvents();
            }

            for (int i = 0; i < tmp2.size(); i++)
            {
                out << tmp2[i][0] << ',' << tmp2[i][1] << ',' << tmp2[i][2] << ',' << tmp2[i][3] << ',';
                out << tmp2[i][13] << ',' << tmp2[i][15] << ',' << tmp2[i][12] << ',' << tmp2[i][16] << ',';
                out << tmp2[i][17] << ',' << tmp2[i][18] << ',' << tmp2[i][11] << ',' << tmp2[i][19] << ',';
                out << tmp2[i][20] << ',' << tmp2[i][9] << ',' << tmp2[i][21] << ',' << tmp2[i][22] << ',';
                out << tmp2[i][23] << ',' << tmp2[i][10] << ',' << tmp2[i][24] << ',' << tmp2[i][4] << ',';
                out << tmp2[i][5] << ',' << tmp2[i][6] << ',' << tmp2[i][7] << ',' << tmp2[i][8] << endl;
            }
        }
        else
        { //軸にdiff DNがない時
            for (int i = 0; i < pixelList2.size(); i++)
            {
                out << pixelList2[i][0] << ',' << pixelList2[i][1] << ',' << pixelList2[i][2] << ',' << pixelList2[i][3] << ',';
                out << pixelList2[i][13] << ',' << pixelList2[i][15] << ',' << pixelList2[i][12] << ',' << pixelList2[i][16] << ',';
                out << pixelList2[i][17] << ',' << pixelList2[i][18] << ',' << pixelList2[i][11] << ',' << pixelList2[i][19] << ',';
                out << pixelList2[i][20] << ',' << pixelList2[i][9] << ',' << pixelList2[i][21] << ',' << pixelList2[i][22] << ',';
                out << pixelList2[i][23] << ',' << pixelList2[i][10] << ',' << pixelList2[i][24] << ',' << pixelList2[i][4] << ',';
                out << pixelList2[i][5] << ',' << pixelList2[i][6] << ',' << pixelList2[i][7] << ',' << pixelList2[i][8] << endl;
            }
        }
    }
}

//カーソルが動かされた時にテキストエディタを変更
void CalibrationGraph::OutputSliderValue(QString fileName)
{
    QString date, time, str;

    date = QDateTime::currentDateTime().date().toString(Qt::ISODate);
    time = QDateTime::currentDateTime().time().toString(Qt::ISODate);

    str = date + "_" + time + "_Slidervalue";
    // QString fileName= QFileDialog::getExistingDirectory(this, tr("Select the directory for Output Slider Value"));
    //  cout<<str.toStdString()<<endl;

    if (fileName == "")
    {
        return;
    }

    QFile file(fileName + "/" + str + ".txt");
    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);

    out << "Target Temperature" << endl;
    out << "Max: " << OutputSliderValuearray[0] << endl;
    out << "Min: " << OutputSliderValuearray[1] << endl;
    out << endl;

    out << "Bolometer Temperature" << endl;
    out << "Max: " << OutputSliderValuearray[2] << endl;
    out << "Min: " << OutputSliderValuearray[3] << endl;
    out << endl;

    out << "Package Temperature" << endl;
    out << "Max: " << OutputSliderValuearray[4] << endl;
    out << "Min: " << OutputSliderValuearray[5] << endl;
    out << endl;

    out << "Case Temperature" << endl;
    out << "Max: " << OutputSliderValuearray[6] << endl;
    out << "Min: " << OutputSliderValuearray[7] << endl;
    out << endl;

    out << "Shutter Temperature" << endl;
    out << "Max: " << OutputSliderValuearray[8] << endl;
    out << "Min: " << OutputSliderValuearray[9] << endl;
    out << endl;

    out << "Lens Temperature" << endl;
    out << "Max: " << OutputSliderValuearray[10] << endl;
    out << "Min: " << OutputSliderValuearray[11] << endl;
    out << endl;
    out << "********************************" << endl;

    out << "Plot Number" << endl;
    out << QString::number(vx.size());
    out << "/";
    out << Outputplotnumberini << endl;

    out << endl;

    //  QString searchName = ui->widget->selectedGraphs().first()->name().section(",",-1,-1);
    //out<<searchName<<endl;

    out << "Used image file" << endl;
    for (int i = 0; i < numfortxt; i++)
    {
        out << Usedimage[i] << endl;
    }
    file.close();
}

void CalibrationGraph::OutputUsedImage(QString fileName)
{
    // QString fileName= QFileDialog::getExistingDirectory(this, tr("Select the directory for Output Slider Value"));
    //  cout<<str.toStdString()<<endl;

    if (fileName == "")
    {
        return;
    }

    QFile file(fileName + "/UsedImage.txt");
    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);

    //  QString searchName = ui->widget->selectedGraphs().first()->name().section(",",-1,-1);
    //out<<searchName<<endl;

    for (int i = 0; i < numfortxt; i++)
    {
        out << Usedimage[i] << endl;
    }
    file.close();
}

//カーソルが動かされた時にテキストエディタを変更
void CalibrationGraph::on_tgtMaxSlider_valueChanged(int value)
{
    ui->tgtMaxLineEdit->setText(QString::number((double)value / 1000));
    OutputSliderValuearray[0] = (double)value / 1000;
}

void CalibrationGraph::on_tgtMinSlider_valueChanged(int value)
{
    ui->tgtMinLineEdit->setText(QString::number((double)value / 1000));
    OutputSliderValuearray[1] = (double)value / 1000;
}

void CalibrationGraph::on_boloMaxSlider_valueChanged(int value)
{
    ui->boloMaxLineEdit->setText(QString::number((double)value / 1000));
    OutputSliderValuearray[2] = (double)value / 1000;
}

void CalibrationGraph::on_boloMinSlider_valueChanged(int value)
{
    ui->boloMinLineEdit->setText(QString::number((double)value / 1000));
    OutputSliderValuearray[3] = (double)value / 1000;
}

void CalibrationGraph::on_pkgMaxSlider_valueChanged(int value)
{
    ui->pkgMaxLineEdit->setText(QString::number((double)value / 1000));
    OutputSliderValuearray[4] = (double)value / 1000;
}

void CalibrationGraph::on_pkgMinSlider_valueChanged(int value)
{
    ui->pkgMinLineEdit->setText(QString::number((double)value / 1000));
    OutputSliderValuearray[5] = (double)value / 1000;
}

void CalibrationGraph::on_caseMaxSlider_valueChanged(int value)
{
    ui->caseMaxLineEdit->setText(QString::number((double)value / 1000));
    OutputSliderValuearray[6] = (double)value / 1000;
}

void CalibrationGraph::on_caseMinSlider_valueChanged(int value)
{
    ui->caseMinLineEdit->setText(QString::number((double)value / 1000));
    OutputSliderValuearray[7] = (double)value / 1000;
}

void CalibrationGraph::on_shMaxSlider_valueChanged(int value)
{
    ui->shMaxLineEdit->setText(QString::number((double)value / 1000));
    OutputSliderValuearray[8] = (double)value / 1000;
}

void CalibrationGraph::on_shMinSlider_valueChanged(int value)
{
    ui->shMinLineEdit->setText(QString::number((double)value / 1000));
    OutputSliderValuearray[9] = (double)value / 1000;
}

void CalibrationGraph::on_lensMaxSlider_valueChanged(int value)
{
    ui->lensMaxLineEdit->setText(QString::number((double)value / 1000));
    OutputSliderValuearray[10] = (double)value / 1000;
}

void CalibrationGraph::on_lensMinSlider_valueChanged(int value)
{
    ui->lensMinLineEdit->setText(QString::number((double)value / 1000));
    OutputSliderValuearray[11] = (double)value / 1000;
}

//テキストエディターが変更された時、カーソルの位置を変更
void CalibrationGraph::on_tgtMaxLineEdit_textChanged(const QString &arg1)
{
    ui->tgtMaxSlider->setValue(arg1.toDouble() * 1000);
}

void CalibrationGraph::on_tgtMinLineEdit_textChanged(const QString &arg1)
{
    ui->tgtMinSlider->setValue(arg1.toDouble() * 1000);
}

void CalibrationGraph::on_boloMaxLineEdit_textChanged(const QString &arg1)
{
    ui->boloMaxSlider->setValue(arg1.toDouble() * 1000);
}

void CalibrationGraph::on_boloMinLineEdit_textChanged(const QString &arg1)
{
    ui->boloMinSlider->setValue(arg1.toDouble() * 1000);
}

void CalibrationGraph::on_pkgMaxLineEdit_textChanged(const QString &arg1)
{
    ui->pkgMaxSlider->setValue(arg1.toDouble() * 1000);
}

void CalibrationGraph::on_pkgMinLineEdit_textChanged(const QString &arg1)
{
    ui->pkgMinSlider->setValue(arg1.toDouble() * 1000);
}

void CalibrationGraph::on_caseMaxLineEdit_textChanged(const QString &arg1)
{
    ui->caseMaxSlider->setValue(arg1.toDouble() * 1000);
}

void CalibrationGraph::on_caseMinLineEdit_textChanged(const QString &arg1)
{
    ui->caseMinSlider->setValue(arg1.toDouble() * 1000);
}

void CalibrationGraph::on_shMaxLineEdit_textChanged(const QString &arg1)
{
    ui->shMaxSlider->setValue(arg1.toDouble() * 1000);
}

void CalibrationGraph::on_shMinLineEdit_textChanged(const QString &arg1)
{
    ui->shMinSlider->setValue(arg1.toDouble() * 1000);
}

void CalibrationGraph::on_lensMaxLineEdit_textChanged(const QString &arg1)
{
    ui->lensMaxSlider->setValue(arg1.toDouble() * 1000);
}

void CalibrationGraph::on_lensMinLineEdit_textChanged(const QString &arg1)
{
    ui->lensMinSlider->setValue(arg1.toDouble() * 1000);
}

//Output Graph Imageが押された時の挙動
void CalibrationGraph::on_outputGraphImageButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"), "", tr("PNG (*.png);;JPG (*.jpg);;PDF (*.pdf)"));

    //fileNameが空の場合は終了
    if (fileName == "")
    {
        return;
    }

    //ファイルの設定
    QFile file(fileName);

    //もし存在してなかったら注意勧告して終了
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::information(this, tr("Unable to open file"), file.errorString());
        return;
    }

    //ファイルの種類に合わせて処理
    if (fileName.section(".", -1, -1) == "png")
    {
        ui->widget->savePng(fileName);
    }
    else if (fileName.section(".", -1, -1) == "jpg")
    {
        ui->widget->saveJpg(fileName);
    }
    else if (fileName.section(".", -1, -1) == "pdf")
    {
        ui->widget->savePdf(fileName);
    }
}

//グラフウィジェット内で右クリックした際の挙動
void CalibrationGraph::contextMenuRequest(QPoint pos)
{
    QMenu *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    //右クリックした際のメニューバーに　Remove selected graph　を追加
    if (ui->widget->selectedGraphs().size() > 0 && !ui->widget->selectedGraphs().contains(ui->widget->graph(0)))
    {
        menu->addAction("Remove selected graph", this, SLOT(removeSelectedGraph()));
    }

    //メニューバーに　Remove all regression formula graphs　を追加
    if (ui->widget->graphCount() > 0)
    {
        menu->addAction("Remove all regression formula graphs", this, SLOT(removeRegressionAllGraphs()));
    }

    menu->popup(ui->widget->mapToGlobal(pos));
}

//グラフウィジェット内の描画されている点、線をクリックした際の挙動
void CalibrationGraph::graphClicked(QCPAbstractPlottable *plottable)
{
    //qDebug() << plottable->name();

    //もしクリックされたグラフが一番最初に描画したもの（黒点、つまりピクセル情報）の場合関数を終了
    if (plottable->name() == "Graph 1")
    {
        return;
    }

    //選択されたグラフの回帰式を表示
    ui->regressionFormulaBrowser->setText(plottable->name().section(",", -2, -2));

    QString ra, rb, rc, rd, re, rf, rg, rh;

    ra = plottable->name().section(",", -10, -10);
    rb = plottable->name().section(",", -9, -9);
    rc = plottable->name().section(",", -8, -8);
    rd = plottable->name().section(",", -7, -7);
    re = plottable->name().section(",", -6, -6);
    rf = plottable->name().section(",", -5, -5);
    rg = plottable->name().section(",", -4, -4);
    rh = plottable->name().section(",", -3, -3);
  /*
    ui->coefficient_a->setText(ra);
    ui->coefficient_b->setText(rb);
    ui->coefficient_c->setText(rc);
    ui->coefficient_d->setText(rd);
    ui->coefficient_e->setText(re);
    ui->coefficient_f->setText(rf);
    ui->coefficient_g->setText(rg);
    ui->coefficient_h->setText(rh);
*/
}

//グラフウィジェットで右クリックした際のメニューバーでremoveSelectedGraphを選択した際の挙動
void CalibrationGraph::removeSelectedGraph()
{
    //選択されているグラフの削除
    if (ui->widget->selectedGraphs().size() > 0)
    {
        ui->widget->removeGraph(ui->widget->selectedGraphs().first());
        ui->widget->replot();
        regressionGraphCount--;
    }
}

//グラフウィジェットで右クリックした際のメニューバーでremoveRegressionAllGraphsを選択した際の挙動
void CalibrationGraph::removeRegressionAllGraphs()
{
    //本当に削除するのかどうかの確認
    QMessageBox msgBox;
    msgBox.setText("Is it really okay to delete all regression formulas?");
    msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
    int ret = msgBox.exec(); //結果がretに格納される

    //OKならピクセル情報のグラフ（黒点のグラフ）以外のすべてのグラフを削除
    if (ret == QMessageBox::Ok)
    {
        for (int i = regressionGraphCount - 1; 1 <= i; i--)
        {
            ui->widget->removeGraph(i);
        }

        regressionGraphCount = 1;

        ui->widget->replot();
    }
}

//グラフウィジェット内の何もない空間をクリックした際の挙動
void CalibrationGraph::mousePress()
{
    //グラフ表示関連をすべて初期化
    ui->regressionFormulaBrowser->clear();
 /*
    ui->coefficient_a->clear();
    ui->coefficient_b->clear();
    ui->coefficient_c->clear();
    ui->coefficient_d->clear();
    ui->coefficient_e->clear();
    ui->coefficient_f->clear();
    ui->coefficient_g->clear();
    ui->coefficient_h->clear();
    */
}

//plotFormulaButtonをクリックした際の挙動
void CalibrationGraph::on_plotFormulaButton_clicked()
{
    double da, db, dc, dd, de, df, dg, dh;

    //入力されている情報を元に係数を格納
    /*
    if (ui->coefficient_a->text() == "")
    {
        da = 0;
    }
    else
    {
        da = ui->coefficient_a->text().toDouble();
    }

    if (ui->coefficient_b->text() == "")
    {
        db = 0;
    }
    else
    {
        db = ui->coefficient_b->text().toDouble();
    }

    if (ui->coefficient_c->text() == "")
    {
        dc = 0;
    }
    else
    {
        dc = ui->coefficient_c->text().toDouble();
    }

    if (ui->coefficient_d->text() == "")
    {
        dd = 0;
    }
    else
    {
        dd = ui->coefficient_d->text().toDouble();
    }

    if (ui->coefficient_e->text() == "")
    {
        de = 0;
    }
    else
    {
        de = ui->coefficient_e->text().toDouble();
    }

    if (ui->coefficient_f->text() == "")
    {
        df = 0;
    }
    else
    {
        df = ui->coefficient_f->text().toDouble();
    }

    if (ui->coefficient_g->text() == "")
    {
        dg = 0;
    }
    else
    {
        dg = ui->coefficient_g->text().toDouble();
    }

    if (ui->coefficient_h->text() == "")
    {
        dh = 0;
    }
    else
    {
        dh = ui->coefficient_h->text().toDouble();
    }
    */

    int size = 10000, num = 7;
    double min = xRangeMin;
    QVector<double> x(size), y(size);
    QString formula = "y = ";
    QString coefficient;
    QString num2 = ui->degreeComboBox->currentText();

    //グラフの名前に式と係数を格納するためにそれぞれを設定
    coefficient.append(QString::number(da) + ",");
    coefficient.append(QString::number(db) + ",");
    coefficient.append(QString::number(dc) + ",");
    coefficient.append(QString::number(dd) + ",");
    coefficient.append(QString::number(de) + ",");
    coefficient.append(QString::number(df) + ",");
    coefficient.append(QString::number(dg) + ",");
    coefficient.append(QString::number(dh) + ",");

    if (7 <= num)
    {
        formula.append(QString::number(da) + "x^7 ");
    }

    if (6 == num)
    {
        formula.append(QString::number(db) + "x^6 ");
    }
    else if (6 <= num)
    {
        if (db < 0)
        {
            formula.append("- " + QString::number(-1 * db) + "x^6 ");
        }
        else
        {
            formula.append("+ " + QString::number(db) + "x^6 ");
        }
    }

    if (5 == num)
    {
        formula.append(QString::number(dc) + "x^5 ");
    }
    else if (5 <= num)
    {
        if (dc < 0)
        {
            formula.append("- " + QString::number(-1 * dc) + "x^5 ");
        }
        else
        {
            formula.append("+ " + QString::number(dc) + "x^5 ");
        }
    }

    if (4 == num)
    {
        formula.append(QString::number(dd) + "x^4 ");
    }
    else if (4 <= num)
    {
        if (dd < 0)
        {
            formula.append("- " + QString::number(-1 * dd) + "x^4 ");
        }
        else
        {
            formula.append("+ " + QString::number(dd) + "x^4 ");
        }
    }

    if (3 == num)
    {
        formula.append(QString::number(de) + "x^3 ");
    }
    else if (3 <= num)
    {
        if (de < 0)
        {
            formula.append("- " + QString::number(-1 * de) + "x^3 ");
        }
        else
        {
            formula.append("+ " + QString::number(de) + "x^3 ");
        }
    }

    if (2 == num)
    {
        formula.append(QString::number(df) + "x^2 ");
    }
    else if (2 <= num)
    {
        if (df < 0)
        {
            formula.append("- " + QString::number(-1 * df) + "x^2 ");
        }
        else
        {
            formula.append("+ " + QString::number(df) + "x^2 ");
        }
    }

    /*

        if(1 == num || num2=="Black_Body"){
            if(num2=="Black_Body")formula.append(QString::number(dg) + "F(T) ");
            else formula.append(QString::number(dg) + "x ");
        }else if(1 <= num){
            if(dg < 0){
                if(num2=="Black_Body")formula.append("- " + QString::number(-1*dg) + "F(T) ");
                else formula.append("- " + QString::number(-1*dg) + "x ");
            }else{
                if(num2=="Black_Body")formula.append("+ " + QString::number(dg) + "F(T) ");
                else formula.append("+ " + QString::number(dg) + "x ");
            }
        }

        */

    if (1 == num || num2 == "Black_Body")
    {
        if (num2 == "Black_Body")
            formula.append(QString::number(dg) + "*F( T + 273.15) ");
        else
            formula.append(QString::number(dg) + "x ");
    }
    else if (1 <= num)
    {
        if (dg < 0)
        {
            if (num2 == "Black_Body")
                formula.append("- " + QString::number(-1 * dg) + "*F( T + 273.15) ");
            else
                formula.append("- " + QString::number(-1 * dg) + "x ");
        }
        else
        {
            if (num2 == "Black_Body")
                formula.append("+ " + QString::number(dg) + "*F( T + 273.15) ");
            else
                formula.append("+ " + QString::number(dg) + "x ");
        }
    }
    if (dh < 0)
    {
        formula.append("- " + QString::number(-1 * dh));
    }
    else
    {
        formula.append("+ " + QString::number(dh));
    }

    //入力されている係数を元に回帰式を描画
    if (num2 == "Black_Body")
    {
        for (int i = 0; i < size; i++)
        {
            x[i] = min;
            y[i] = dg * planck(min + 273.15) + dg * planck(min + 273.15) - dh;
            min += (xRangeMax - xRangeMin) / 10000;
        }
    }
    else
    {
        for (int i = 0; i < size; i++)
        {
            x[i] = min;
            y[i] = da * pow(min, 7) + db * pow(min, 6) + dc * pow(min, 5) + dd * pow(min, 4) + de * pow(min, 3) + df * pow(min, 2) + dg * min + dh;
            min += (xRangeMax - xRangeMin) / 10000;
        }
    }

    //ペアを探す処理
    QVector<QString> tmp1;
    int searchID, pairID;

    for (int i = 0; i < replot.size(); i++)
    {
        //16進数の文字列を10進数に変換
        searchID = replot[i][10].toInt(0, 16);

        //IDが奇数のときの処理
        if (searchID % 2)
        {
            pairID = searchID + 1;
        }
        else
        {
            //IDが偶数の時の処理
            pairID = searchID - 1;
        }

        //pixelListの中からファイル名が一致したピクセルのファイル名を格納
        for (int j = 0; j < pixelList.size(); j++)
        {
            if (pixelList[j][0] == replot[i][0])
            {
                tmp1.append(pixelList[j][0]);
                break;
            }
        }

        //ペアとなるピクセルのファイル名を格納
        for (int j = 0; j < pixelList.size(); j++)
        {
            if (pixelList[j][10].toInt(0, 16) == pairID && 
            replot[i][11] == pixelList[j][11] && 
            replot[i][12] == pixelList[j][12])
            {
                tmp1.append(pixelList[j][0]);
                break;
            }
        }
    }

    //絞り込まれたファイル名を取得
    QString searchName;
    searchName.clear();
    for (int l = 0; l < tmp1.size(); l++)
    {
        searchName.append("tirimageinfo.img_file='" + tmp1[l] + "'");
        if (l + 1 != tmp1.size())
        {
            searchName.append(" or ");
        }
    }

    ui->widget->addGraph(); //回帰用のグラフの追加
    ui->widget->graph(regressionGraphCount)->setData(x, y); //グラフデータをセット
    ui->widget->graph(regressionGraphCount)->setPen(QPen(Qt::red)); //色の設定
    ui->widget->graph(regressionGraphCount)->setName(QString::number(num) + "," + coefficient + formula + "," + searchName); //グラフの名前を格納
    ui->widget->replot(); //再描画

    regressionGraphCount++;
}

void CalibrationGraph::on_exportFormulaButton_clicked() //指定された範囲をキャリブレーション
{
    int xmin, xmax, ymin, ymax;

    xmin = ui->MinxlineEdit->text().toInt();
    xmax = ui->MaxxlineEdit->text().toInt();
    ymin = ui->MinylineEdit->text().toInt();
    ymax = ui->MaxylineEdit->text().toInt();

    int upperleft=judgeTableNameint(xmin,ymin);
    int upperright=judgeTableNameint(xmax,ymin);
    int lowerleft=judgeTableNameint(xmin,ymax);
    int lowerright=judgeTableNameint(xmax,ymax);
    int tablecounter=upperleft;
    QVector <QString>table;
    int width=upperright-upperleft;
    int tablecounter3=upperleft;
    while(1){
        if(lowerleft==tablecounter3)
        {
            for(int p=lowerleft;p<=lowerright;p++){
                if(tablecounter<10){
                    table.append("pix0"+QString::number(tablecounter));
                }
                else{
                    table.append("pix"+QString::number(tablecounter));
                }
                tablecounter++;
            }
            break;
        }

        int tablecounter2=1;
        while(1){
            if(tablecounter<10){
                table.append("pix0"+QString::number(tablecounter));
            }
            else{
                table.append("pix"+QString::number(tablecounter));
            }
            if(tablecounter2==width+1)break;

            tablecounter++;
            tablecounter2++;
        }

        tablecounter=tablecounter3+12;
        tablecounter3=tablecounter3+12;
    }
    //本当に登録して良いのかの確認
    //QString basedir = QCoreApplication::applicationDirPath()+"/";
    QString date, time;
    date = QDateTime::currentDateTime().date().toString(Qt::ISODate);
    time = QDateTime::currentDateTime().time().toString(Qt::ISODate);
    QString basedir = "/Applications/HEATcalibration/";
    QString subdir = 
    date + "_" + time + "_" + QString::number(vx.size()) + "grounddata";

    QDir dir1(basedir);
    if (dir1.exists(subdir))
    {
        QMessageBox msgBox;
        msgBox.setText("This calibration formula already exists. Is it okay to export anyway?");
        msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
        int ret = msgBox.exec();
        if (ret == QMessageBox::Cancel)
            return;
    }

    else
    {
        QMessageBox msgBox;
        msgBox.setText("Is it okay to export this formula?");
        msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);

        int ret = msgBox.exec(); //結果がretに格納される

        //キャンセルの場合は終了
        if (ret != QMessageBox::Ok)
        {
            return;
        }
    }

    //回帰式が適切でない場合は終了
    if (ui->regressionFormulaBrowser->toPlainText() == "" || ui->regressionFormulaBrowser->toPlainText() == "Graph 1" || ui->regressionFormulaBrowser->toPlainText() == "The graph is not appropriate.")
    {
        return;
    }

    //縦軸と横軸が同名の場合は終了
    if (xAxis == yAxis)
    {
        return;
    }

    //縦軸または横軸がNo.の場合は終了
    if (xAxis == "No." || yAxis == "No.")
    {
        return;
    }

    //プログレスダイアログの設定
    QProgressDialog p;
    p.setLabelText("Registration Progress");
    p.setRange(0, 248 * 328);
    p.show();
    QCoreApplication::processEvents();

    //残差配列の初期化

    for (int i = 0; i < 248; i++)
    {
        for (int j = 0; j < 328; j++)
        {
            residual[i][j] = "non";
            arra[i][j] = "non";
            arrb[i][j] = "non";
            arrc[i][j] = "non";
            arrd[i][j] = "non";
            arre[i][j] = "non";
            arrf[i][j] = "non";
            arrg[i][j] = "non";
            arrh[i][j] = "non";
        }
    }

    //ペアを探す処理
    QVector<QString> tmpfortxt;
    int searchIDfortxt, pairIDfortxt;
    for (int i = 0; i < replot.size(); i++)
    {
        //16進数の文字列を10進数に変換
        searchIDfortxt = replot[i][10].toInt(0, 16);

        //IDが奇数のときの処理
        if (searchIDfortxt % 2)
        {
            pairIDfortxt = searchIDfortxt + 1;
        }
        else
        {
            //IDが偶数の時の処理
            pairIDfortxt = searchIDfortxt - 1;
        }

        //pixelListの中からファイル名が一致したピクセルのファイル名を格納
        for (int j = 0; j < pixelList.size(); j++)
        {
            if (pixelList[j][0] == replot[i][0])
            {
                tmpfortxt.append(pixelList[j][0]);
                break;
            }
        }

        //ペアとなるピクセルのファイル名を格納
        for (int j = 0; j < pixelList.size(); j++)
        {
            if (pixelList[j][10].toInt(0, 16) == pairIDfortxt && replot[i][11] == pixelList[j][11] && replot[i][12] == pixelList[j][12])
            {
                tmpfortxt.append(pixelList[j][0]);
                break;
            }
        }
    }

    //絞り込まれたファイル名を取得
    numfortxt = tmpfortxt.size();
    for (int l = 0; l < tmpfortxt.size(); l++)
    {
        Usedimage[l] = tmpfortxt[l];
    }

    //qDebug() << searchName;

    //絞り込まれたファイル名を連結させて取得
    QString searchName = ui->widget->selectedGraphs().first()->name().section(",", -1, -1);

    connectDB(); //データベース接続

    //熱較正式式ファイル読み込み
    QDir tmp; //データパス用

    //パス
    //initialFileDirectory.cd("Calibration");

    //校正式を登録するフォルダを作成
    QDir dir2(basedir);
    if (!dir2.exists(subdir))
        dir2.mkdir(subdir);

    //校正式の登録先選択
    //    QString initialFileDirectory = QFileDialog::getExistingDirectory(this,"Select Register Formula Folder");
    QString initialFileDirectory = basedir + subdir;

    if (initialFileDirectory == "")
    {
        return;
    }

    initialFileDirectory_thread = initialFileDirectory;
    OutputSliderValue(initialFileDirectory);
    OutputUsedImage(initialFileDirectory);

    //ファイルを作成して較正式を登録
    QString searchObject = "";
    QString tableName = "";
    QVector<QVector<QString>> tmp2;
    QVector<QVector<QString>> tmp3;
    QVector<QString> t;
    int x, y;
    int searchID, pairID;

    if (xAxis == "No.")
    {
        searchObject.append("img_file, ");
    }
    else if (xAxis == "pkg T(degC)")
    {
        searchObject.append("pkg_t_mon, ");
    }
    else if (xAxis == "case T(degC)")
    {
        searchObject.append("case_t_mon, ");
    }
    else if (xAxis == "shtr T(degC)")
    {
        searchObject.append("sh_t_mon, ");
    }
    else if (xAxis == "lens T(degC)")
    {
        searchObject.append("lens_t_mon, ");
    }
    else if ((xAxis == "open DN" || xAxis == "close DN" || xAxis == "diff DN") && ismodifiedgl == false)
    {
        searchObject.append("pixel, ");
        // qDebug() << "isnotmo";
    }
    else if ((xAxis == "open DN" || xAxis == "close DN" || xAxis == "diff DN") && ismodifiedgl == true)
    {
        searchObject.append("pixel_modified, ");
        //qDebug() << "ismo";
    }
    else if (xAxis == "target T(degC)")
    {
        searchObject.append("target_t, ");
    }

    if (yAxis == "No.")
    {
        searchObject.append("img_file, ");
    }
    else if (yAxis == "pkg T(degC)")
    {
        searchObject.append("pkg_t_mon, ");
    }
    else if (yAxis == "case T(degC)")
    {
        searchObject.append("case_t_mon, ");
    }
    else if (yAxis == "shtr T(degC)")
    {
        searchObject.append("sh_t_mon, ");
    }
    else if (yAxis == "lens T(degC)")
    {
        searchObject.append("lens_t_mon, ");
    }
    else if ((yAxis == "open DN" || yAxis == "close DN" || yAxis == "diff DN") && ismodifiedgl == false)
    {
        searchObject.append("pixel, ");
        //  qDebug() << "isnotmo";
    }
    else if ((yAxis == "open DN" || yAxis == "close DN" || yAxis == "diff DN") && ismodifiedgl == true)
    {
        searchObject.append("pixel_modified, ");
        // qDebug() << "ismo";
    }
    else if (yAxis == "target T(degC)")
    {
        searchObject.append("target_t, ");
    }
    QString querystring;
    //    for(int i=1; i<97; i++){
    int i = 0;
    //for (i = 1; i < 97; i++)

        for(i=0;i<table.length();i++)
    {
        tmp2.clear();
        tmp2_thread.clear();
        if (!(table[i] == "pix12" || table[i] == "pix24" || table[i] == "pix36" || table[i] == "pix48" || table[i] == "pix60" || table[i] == "pix72" || table[i] == "pix84" || table[i] == "pix96"))
        { //計算がいらないのでスキップ
            querystring = "SELECT " + table[i] + ".img_file, x, y," + searchObject + "img_id, target_name, path, m, mask FROM " + table[i] + ", tirimageinfo " +
                        "WHERE tirimageinfo.img_file=" + table[i] + ".img_file AND (" + searchName + ") AND x>=16 AND x<=343 AND y>=6 AND y<=253";

            // qDebug() << querystring;

            //プログレス表示
            //  qDebug() << "Registration Progress " + QString::number(i) + "/96 ";

            //0  img_file
            //1  x座標
            //2  y座標
            //3  target_t   pixel (pixel_modified)
            //4  pixel (pixel_modified)   target_t
            //5  img_id
            //6  tarhet_name
            //7  path
            //8  m
            //9 mask
            query.clear();
            query.exec(querystring);
            int pixelxy;

            if (table[i] == "pix01")
            { //16*26
                pixelxy = 416;
            }
            else if (table[i] == "pix11")
            { //24*26
                pixelxy = 624;
            }
            else if (table[i] == "pix85")
            { //16*30
                pixelxy = 480;
            }
            else if (table[i] == "pix95")
            { //24*30
                pixelxy = 720;
            }

            else if (table[i] == "pix02" || table[i] == "pix03" || table[i] == "pix04" || table[i] == "pix05" || table[i] == "pix06" || table[i] == "pix07" || table[i] == "pix08" || table[i] == "pix09" || table[i] == "pix10") //32*26
            {
                pixelxy = 832;
            }

            else if (table[i] == "pix86" || table[i] == "pix87" || table[i] == "pix88" || table[i] == "pix89" || table[i] == "pix90" || table[i] == "pix91" || table[i] == "pix92" || table[i] == "pix93" || table[i] == "pix94") //32*30
            {
                pixelxy = 960;
            }
            else if (table[i] == "pix13" || table[i] == "pix25" || table[i] == "pix37" || table[i] == "pix49" || table[i] == "pix61" || table[i] == "pix73") //16*32
            {
                pixelxy = 512;
            }
            else if (table[i] == "pix23" || table[i] == "pix35" || table[i] == "pix47" || table[i] == "pix59" || table[i] == "pix71" || table[i] == "pix83") //24*32
            {
                pixelxy = 768;
            }
            else
            {
                pixelxy = 1024;
            }

            pixelxy_thread = pixelxy;
            xAxis_thread = xAxis;
            yAxis_thread = yAxis;
            num2_thread = ui->degreeComboBox->currentText();
            int counter = 0;
            int xaxis[pixelxy];
            int yaxis[pixelxy];

            query.first();
            do
            {
                t.clear();
                t.append(query.value(0).toString()); // img_file

                if(query.value(8).toInt() > 1 && (xAxis == "diff DN" || xAxis == "open DN" || xAxis == "close DN"))
                { // 積算枚数が１以上の時は８で割る
                    t.append(QString::number(query.value(3).toDouble() / 8)); // x軸に格納されている情報の値
                }
                else
                {
                    t.append(query.value(3).toString()); // x軸に格納されている情報の値
                }

                if (query.value(8).toInt() > 1 && (yAxis == "diff DN" || yAxis == "open DN" || yAxis == "close DN"))
                { // 積算枚数が１以上の時は８で割る
                    t.append(QString::number(query.value(4).toDouble() / 8)); // y軸に格納されている情報の値
                }
                else
                {
                    t.append(query.value(4).toString()); // y軸に格納されている情報の値
                }
                t.append(query.value(5).toString()); // img_id
                t.append(query.value(6).toString()); // target_name
                t.append(query.value(7).toString()); // path
                t.append(query.value(9).toString()); // mask

                tmp2.append(t);
                tmp2_thread.append(t);
                if (counter < pixelxy)
                {
                    xaxis[counter] = query.value(1).toInt();        // x座標
                    yaxis[counter] = query.value(2).toInt();        // y座標
                    xaxis_thread[counter] = query.value(1).toInt(); // x座標
                    yaxis_thread[counter] = query.value(2).toInt(); // y座標
                }
                counter++;

            } while (query.next());
            t.clear();
            Outputplotnumber_thread = Outputplotnumber_thread = tmpfortxt.length()/2;
if(tmp2_thread.size()>1){
    qDebug()<<"threaddekiteruyo";
#pragma omp parallel for
    for (int pix = 0; pix <pixelxy_thread; pix++)
    {
        int searchID, pairID;
        QVector<QString> t;
        QVector<QVector<QString>> tmp3;
        QDir tmp5;
        //qDebug()<<"<<pix;
        int xycounter = pix;
        for (int plotn = 0; plotn < Outputplotnumber_thread * 2; plotn++)
        {
            //qDebug()<<plotn;
            //qDebug()<<"<plotn"<<plotn;
            //  qDebug()<<tmp2[pixelxy*plotn+pix];//lをpixelxy回回すと同一ピクセルにもどる
            int currentpixel = pixelxy_thread * plotn + pix;
            if (tmp2_thread[currentpixel][0].section('.', 2, 2) == "open" && tmp2_thread[currentpixel][6]=="1")
            {
                // qDebug()<<"mask1";
                searchID = tmp2_thread[currentpixel][3].toInt(0, 16);
                //IDが奇数のときの処理
                if (searchID % 2)
                {
                    pairID = searchID + 1;
                }
                else
                {
                    //IDが偶数の時の処理
                    pairID = searchID - 1;
                }
                //qDebug()<<"ghk";
                //基本はopenの情報を格納しつつ、DN値のみ差分を格納
                for (int plotn2 = 0; plotn2 < Outputplotnumber_thread * 2; plotn2++)
                {
                    //     qDebug()<<"plotn2"<<plotn2;
                    int currentpixel2 = pixelxy_thread * plotn2 + pix;
                    if (tmp2_thread[currentpixel2][3].toInt(0, 16) == pairID && tmp2_thread[currentpixel][4] == tmp2_thread[currentpixel2][4] && tmp2_thread[currentpixel][5] == tmp2_thread[currentpixel2][5])
                    {
                        t.clear();
                        t.append(tmp2_thread[pixelxy_thread * plotn + pix][0]);
                        if (xAxis_thread == "diff DN")
                        {
                            t.append(QString::number(tmp2_thread[currentpixel][1].toDouble() - tmp2_thread[currentpixel2][1].toDouble()));
                            t.append(tmp2_thread[pixelxy_thread * plotn + pix][2]);
                        }
                        else if (yAxis_thread == "diff DN")
                        {
                            t.append(tmp2_thread[pixelxy_thread * plotn + pix][1]);
                            t.append(QString::number(tmp2_thread[currentpixel][2].toDouble() - tmp2_thread[currentpixel2][2].toDouble()));
                        }
                        tmp3.append(t);
                        break;
                    }
                }
            }                                //qDebug()<<"mask0";
        }

        //ファイル作成処理
        //それぞれvx、vyに格納
        QVector<double> vx;
        QVector<double> vy;
        if (yAxis_thread == "diff DN" || xAxis_thread == "diff DN")
        {
            for (int l = 0; l < tmp3.size(); l++)
            {
                vx.append(tmp3[l][1].toDouble());
                vy.append(tmp3[l][2].toDouble());
            }
        }
        else
        {
            for (int l = 0; l < tmp2_thread.size(); l++)
            {
                vx.append(tmp2_thread[l][1].toDouble());
                vy.append(tmp2_thread[l][2].toDouble());
            }
        }


        QString s;
        //回帰係数を計算し、登録の際に使用するSringを取得
        //   s = getRegressionCoefficientInRegisterRegressionforBlackbody(vx, vy, xaxis_thread[xycounter]-16, yaxis_thread[xycounter]-6);

        int num;
        double x = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0, x6 = 0, x7 = 0, x8 = 0, x9 = 0, x10 = 0, x11 = 0, x12 = 0, x13 = 0, x14 = 0;
        double x7y = 0, x6y = 0, x5y = 0, x4y = 0, x3y = 0, x2y = 0, xy = 0, y = 0;
        double pivot, mul;
        double aa, bb, cc, dd, ee, ff, gg, hh;
        QString coefficient, formula;
        //次数の取得 & +1の処理
        //num = ui->widget->selectedGraphs().at(0)->name().section(",",0,0).toInt() + 1;
        num = 2;
        //qDebug() << num;

        //係数の初期化
        aa = bb = cc = dd = ee = ff = gg = hh = 0;

        //それぞれの総和
        for (int i = 0; i < vx.size(); i++)
        {
            x += planck4(vx[i] + 273.15);
            x2 += pow(planck4(vx[i] + 273.15), 2);
            x3 += pow(planck4(vx[i] + 273.15), 3);
            x4 += pow(planck4(vx[i] + 273.15), 4);
            x5 += pow(planck4(vx[i] + 273.15), 5);
            x6 += pow(planck4(vx[i] + 273.15), 6);
            x7 += pow(planck4(vx[i] + 273.15), 7);
            x8 += pow(planck4(vx[i] + 273.15), 8);
            x9 += pow(planck4(vx[i] + 273.15), 9);
            x10 += pow(planck4(vx[i] + 273.15), 10);
            x11 += pow(planck4(vx[i] + 273.15), 11);
            x12 += pow(planck4(vx[i] + 273.15), 12);
            x13 += pow(planck4(vx[i] + 273.15), 13);
            x14 += pow(planck4(vx[i] + 273.15), 14);
            x7y += (pow(planck4(vx[i] + 273.15), 7) * vy[i]);
            x6y += (pow(planck4(vx[i] + 273.15), 6) * vy[i]);
            x5y += (pow(planck4(vx[i] + 273.15), 5) * vy[i]);
            x4y += (pow(planck4(vx[i] + 273.15), 4) * vy[i]);
            x3y += (pow(planck4(vx[i] + 273.15), 3) * vy[i]);
            x2y += (pow(planck4(vx[i] + 273.15), 2) * vy[i]);
            xy += (planck4(vx[i] + 273.15) * vy[i]);
            y += vy[i];
            /*   cout<<"T"<<endl;
                                         cout<<vx[i]<<endl;

                                            cout<<"F"<<endl;
                                            cout<<planck(vx[i]+273.15)<<endl;
                                    */
        }

        // 拡大係数行列 M
        double M[8][8 + 1] =
        {
            {x14, x13, x12, x11, x10, x9, x8, x7, x7y},
            {x13, x12, x11, x10, x9, x8, x7, x6, x6y},
            {x12, x11, x10, x9, x8, x7, x6, x5, x5y},
            {x11, x10, x9, x8, x7, x6, x5, x4, x4y},
            {x10, x9, x8, x7, x6, x5, x4, x3, x3y},
            {x9, x8, x7, x6, x5, x4, x3, x2, x2y},
            {x8, x7, x6, x5, x4, x3, x2, x, xy},
            {x7, x6, x5, x4, x3, x2, x, double(vx.size()), y}};

        double **M2 = new double *[num];
        for (int i = 0; i < num; i++)
        {
            M2[i] = new double[num + 1];
        }

        for (int i = 0; i < num; i++)
        {
            for (int j = 0; j < num + 1; j++)
            {
                M2[i][j] = M[(8 - num) + i][(8 - num) + j];
            }
        }

        // 対角成分が1で正規化された階段行列を作る(前進消去)
        for (int i = 0; i < num; ++i)
        {
            // 対角成分の選択、この値で行成分を正規化
            pivot = M2[i][i];
            for (int j = 0; j < num + 1; ++j)
            {
                M2[i][j] = (1 / pivot) * M2[i][j];
            }

            // 階段行列を作る為に、現在の行より下の行について
            // i列目の成分が0になるような基本変形をする
            for (int k = i + 1; k < num; ++k)
            {
                mul = M2[k][i];
                for (int count = i; count < num + 1; ++count)
                {
                    M2[k][count] = M2[k][count] - mul * M2[i][count];
                }
            }
        }

        // 下から上に向かって変数に代入して、独立した解の形にする(後進代入)
        // このとき一番下の行はすでに独立した解を得ている
        for (int i = num - 1; i > 0; --i)
        {
            for (int k = i - 1; k >= 0; --k)
            {
                mul = M2[k][i];
                for (int count = i; count < num + 1; ++count)
                {
                    M2[k][count] = M2[k][count] - mul * M2[i][count];
                }
            }
        }

        //対応する係数を格納
        if (1 < num)
        {
            hh = M2[num - 1][num];
            gg = M2[num - 2][num];
        }
        if (2 < num)
        {
            ff = M2[num - 3][num];
        }
        if (3 < num)
        {
            ee = M2[num - 4][num];
        }
        if (4 < num)
        {
            dd = M2[num - 5][num];
        }
        if (5 < num)
        {
            cc = M2[num - 6][num];
        }
        if (6 < num)
        {
            bb = M2[num - 7][num];
        }
        if (7 < num)
        {
            aa = M2[num - 8][num];
        }

        //メモリの解放
        for (int i = 0; i < num; i++)
        {
            delete[] M2[i];
        }
        delete[] M2;

        //残差を求める
        double d = 0;
        double tmp;
        for (int i = 0; i < vx.size(); i++)
        {
            tmp = vy[i] - (gg * vx[i] + hh);
            if (tmp < 0)
            {
                d += tmp * -1;
            }
            else
            {
                d += tmp;
            }
        }
        d = d / vx.size();
        //残差配列に座標に対応する残差を格納
        residual[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(d);
        arra[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(aa);
        arrb[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(bb);
        arrc[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(cc);
        arrd[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(dd);
        arre[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(ee);
        arrf[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(ff);
        arrg[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(gg);
        arrh[yaxis_thread[xycounter] - 6][xaxis_thread[xycounter] - 16] = QString::number(hh);

        saveFileInRegisterRegression("residual.csv",initialFileDirectory_thread);

        saveFileInRegisterRegression("a.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("b.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("c.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("d.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("e.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("f.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("g.csv",initialFileDirectory_thread);
        saveFileInRegisterRegression("h.csv",initialFileDirectory_thread);
        //ファイルに記述する際に使用する文を作成

        coefficient.append(QString::number(aa) + ",");
        coefficient.append(QString::number(bb) + ",");
        coefficient.append(QString::number(cc) + ",");
        coefficient.append(QString::number(dd) + ",");
        coefficient.append(QString::number(ee) + ",");
        coefficient.append(QString::number(ff) + ",");
        coefficient.append(QString::number(gg) + ",");
        coefficient.append(QString::number(hh) + ",");

        num = num - 1;

        if (2 == num)
        {
            formula.append(QString::number(ff) + "x^2 ");
        }
        else if (2 <= num)
        {
            if (ff < 0)
            {
                formula.append("- " + QString::number(-1 * ff) + "x^2 ");
            }
            else
            {
                formula.append("+ " + QString::number(ff) + "x^2 ");
            }
        }

        if (1 == num || num2_thread == "Black_Body")
        {
            if (num2_thread == "Black_Body")
                formula.append(QString::number(gg) + "*F( T + 273.15) ");
            else
                formula.append(QString::number(gg) + "x ");
        }
        else if (1 <= num)
        {
            if (gg < 0)
            {
                if (num2_thread == "Black_Body")
                    formula.append("- " + QString::number(-1 * gg) + "*F( T + 273.15) ");
                else
                    formula.append("- " + QString::number(-1 * gg) + "x ");
            }
            else
            {
                if (num2_thread == "Black_Body")
                    formula.append("+ " + QString::number(gg) + "*F( T + 273.15) ");
                else
                    formula.append("+ " + QString::number(gg) + "x ");
            }
        }


        if (hh < 0)
        {
            formula.append("- " + QString::number(-1 * hh));
        }
        else
        {
            formula.append("+ " + QString::number(hh));
        }

        s = QString::number(num) + "," + coefficient + formula;

        //取得した登録内容をファイルに書き込み
        QFile file(initialFileDirectory_thread + "/" + QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");
        vx.clear();
        vy.clear();
        tmp3.clear();
        //ファイルが存在する場合
        if (file.exists())
        {
            file.open(QIODevice::WriteOnly);
            QTextStream out(&file);

            out << s << endl;

            file.close();
        }
        else
        { //ファイルが存在しなかった場合
            //ファイル作成
            //QFile ini(QString::number(x-16) + "_" + QString::number(y-6) + "_" + tmp1[l]+ ".txt");
            QFile ini(QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");

            ini.open(QIODevice::WriteOnly);

            QTextStream out(&ini);

            out << s << endl;

            ini.close();

            tmp5.rename(QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt",
                        initialFileDirectory_thread + "/" + QString::number(xaxis_thread[xycounter] - 16) + "_" + QString::number(yaxis_thread[xycounter] - 6) + ".txt");
        }
        tmp3.clear();
    }
}
            
            /*
            //画像ごとに得たクエリをピクセルごとにソーティング
            for(int pix=0; pix<pixelxy; pix++)
            {
                //    qDebug()<<"pix"<<pix;
                for(int plotn=0; plotn<Outputplotnumber*2; plotn++)
                {
                    //qDebug()<<"<plotn"<<plotn;
                    //  qDebug()<<tmp2[pixelxy*plotn+pix];//lをpixelxy回回すと同一ピクセルにもどる
                    int currentpixel=pixelxy*plotn+pix;
                    if(tmp2[currentpixel][0].section('.',2,2) == "open")
                    {
                        //                  qDebug()<<"def";
                        searchID = tmp2[currentpixel][3].toInt(0,16);

                        //IDが奇数のときの処理
                        if(searchID%2){
                            pairID = searchID + 1;
                        }else{
                            //IDが偶数の時の処理
                            pairID = searchID - 1;
                        }

                        //                    qDebug()<<"ghk";
                        //基本はopenの情報を格納しつつ、DN値のみ差分を格納
                        for(int plotn2=0; plotn2<Outputplotnumber*2; plotn2++){
                            //     qDebug()<<"plotn2"<<plotn2;
                            int currentpixel2=pixelxy*plotn2+pix;
                            if(tmp2[currentpixel2][3].toInt(0,16)==pairID && tmp2[currentpixel][4]==tmp2[currentpixel2][4] && tmp2[currentpixel][5]==tmp2[currentpixel2][5]){
                                t.clear();
                                t.append(tmp2[pixelxy*plotn+pix][0]);
                                if(xAxis == "diff DN"){
                                    t.append(QString::number(tmp2[currentpixel][1].toDouble() - tmp2[currentpixel2][1].toDouble()));
                                    t.append(tmp2[pixelxy*plotn+pix][2]);
                                }else if(yAxis == "diff DN"){
                                    t.append(tmp2[pixelxy*plotn+pix][1]);
                                    t.append(QString::number(tmp2[currentpixel][2].toDouble() - tmp2[currentpixel2][2].toDouble()));
                                }
                                tmp3.append(t);
                                break;
                            }
                        }
                    }
                }

                //ファイル作成処理
                //それぞれvx、vyに格納
                QVector<double> vx;
                QVector<double> vy;
                if(yAxis == "diff DN" || xAxis == "diff DN"){
                    for(int l=0; l<tmp3.size(); l++){
                        vx.append(tmp3[l][1].toDouble());
                        vy.append(tmp3[l][2].toDouble());

                    }
                }else{
                    for(int l=0; l<tmp2.size(); l++){
                        vx.append(tmp2[l][1].toDouble());
                        vy.append(tmp2[l][2].toDouble());
                    }
                }


                qDebug()<<"xaxis"<<xaxis[xycounter]-16;
                qDebug()<<"yaxis"<<yaxis[xycounter]-6;

                QString s;
                //回帰係数を計算し、登録の際に使用するSringを取得
                s = getRegressionCoefficientInRegisterRegressionforBlackbody(vx, vy, xaxis[xycounter]-16, yaxis[xycounter]-6);

                //取得した登録内容をファイルに書き込み
                QFile file(initialFileDirectory+"/" + QString::number(xaxis[xycounter]-16) + "_" + QString::number(yaxis[xycounter]-6) + ".txt");
                vx.clear();
                vy.clear();
                tmp3.clear();
                //ファイルが存在する場合
                if(file.exists()){
                    file.open(QIODevice::WriteOnly);
                    QTextStream out(&file);

                    out << s << endl;

                    file.close();

                }else{ //ファイルが存在しなかった場合
                    //ファイル作成
                    //QFile ini(QString::number(x-16) + "_" + QString::number(y-6) + "_" + tmp1[l]+ ".txt");
                    QFile ini(QString::number(xaxis[xycounter]-16) + "_" + QString::number(yaxis[xycounter]-6) + ".txt");

                    ini.open(QIODevice::WriteOnly);

                    QTextStream out(&ini);

                    out << s << endl;

                    ini.close();

                    tmp.rename(QString::number(xaxis[xycounter]-16) + "_" + QString::number(yaxis[xycounter]-6) + ".txt",
                               initialFileDirectory+ "/" + QString::number(xaxis[xycounter]-16) + "_" + QString::number(yaxis[xycounter]-6) + ".txt");

                }
                xycounter++;
            }

*/

            //キャンセルボタンが押された場合、関数を終了

            if (p.wasCanceled())
            {

                QProgressDialog p;
                p.setLabelText("Save Registration Information File Process");
                p.setRange(0, 12);
                p.show();
                QCoreApplication::processEvents();

                //ファイル名が空白なら終了
                if (initialFileDirectory == "")
                {
                    return;
                }
            }
        }
    }

    //グラフを全部、赤色に設定
    for (int iii = 1; iii < regressionGraphCount; iii++)
    {
        ui->widget->graph(iii)->setPen(QPen(Qt::red));
    }
    ui->widget->selectedGraphs().first()->setPen(QPen(Qt::darkGreen)); //登録した（選択されている）グラフを緑色に変更

    db.close();

    QMessageBox msgBox2;
    msgBox2.setText("This formula registration have been complete.");
    msgBox2.setStandardButtons(QMessageBox::Ok);
    msgBox2.exec();
}

///////////////////////////////////////////////////////

//全ピクセルにおいて同じ絞り込み（同じファイル名）で回帰を行いファイルに書き込み

/*
                                    QVector<QVector <QString> > tmp2;
                                    QVector<QVector <QString> > tmp3;
                                    QVector<QString> t;

                                    int count=0;
                                    for(int i=xmin; i<xmax+1; i++){//328
                                        for(int j=ymin; j<ymax+1; j++){//248
                                            t.clear();
                                            tmp2.clear();
                                            tmp3.clear();

                                            //x座標、y座標の設定
                                            x = i + 16;
                                            y = j + 6;

                                            //テーブル名の判断
                                            tableName = judgeTableName(x,y);

                                            //クエリー実行
                                            query.exec("SELECT " + tableName + ".img_file, x, y," + searchObject +"img_id, target_name, path, m FROM " + tableName + ", tirimageinfo " +
                                                       "WHERE tirimageinfo.img_file=" + tableName + ".img_file and (" + searchName + ") AND x=" + QString::number(x) + " AND y=" + QString::number(y) + " AND mask=1");


                                            QString aaaaa = "SELECT " + tableName + ".img_file, x, y," + searchObject +"img_id, target_name, path, m FROM " + tableName + ", tirimageinfo " +
                                                    "WHERE tirimageinfo.img_file=" + tableName + ".img_file and (" + searchName + ") AND x=" + QString::number(x) + " AND y=" + QString::number(y) + " AND mask=1";
                                            cout<<aaaaa.toStdString()<<endl;

                                            //qDebug() << "query execute " + tableName + " " + QString::number(x) + " " + QString::number(y);

                                            //クエリーで得たデータを配列に格納
                                            query.first();
                                            do {
                                                t.clear();
                                                t.append(query.value(0).toString()); // img_file

                                                if(query.value(8).toInt() > 1 && (xAxis == "diff DN" || xAxis == "open DN" || xAxis == "close DN")){ // 積算枚数が１以上の時は８で割る
                                                    t.append(QString::number(query.value(3).toDouble() / 8)); // x軸に格納されている情報の値
                                                }else{
                                                    t.append(query.value(3).toString()); // x軸に格納されている情報の値
                                                }

                                                if(query.value(8).toInt() > 1 && (yAxis == "diff DN" || yAxis == "open DN" || yAxis == "close DN")){ // 積算枚数が１以上の時は８で割る                                      
                                                    t.append(QString::number(query.value(4).toDouble() / 8)); // y軸に格納されている情報の値
                                                }else{
                                                    t.append(query.value(4).toString()); // y軸に格納されている情報の値
                                                }
                                                t.append(query.value(5).toString()); // img_id
                                                t.append(query.value(6).toString()); // target_name
                                                t.append(query.value(7).toString()); // path
                                                tmp2.append(t);
                                            }while(query.next());

                                            if(yAxis == "diff DN" || xAxis == "diff DN"){
                                                //クエリーから得た配列内でペアを探し、差分を取得
                                                for(int l=0; l<tmp2.size(); l++){
                                                    if(tmp2[l][0].section('.',2,2) == "open"){
                                                        //16進数の文字列を10進数に変換
                                                        searchID = tmp2[l][3].toInt(0,16);

                                                        //IDが奇数のときの処理
                                                        if(searchID%2){
                                                            pairID = searchID + 1;
                                                        }else{
                                                            //IDが偶数の時の処理
                                                            pairID = searchID - 1;
                                                        }

                                                        //基本はopenの情報を格納しつつ、DN値のみ差分を格納
                                                        for(int l2=0; l2<tmp2.size(); l2++){
                                                            if(tmp2[l2][3].toInt(0,16)==pairID && tmp2[l][4]==tmp2[l2][4] && tmp2[l][5]==tmp2[l2][5]){
                                                                t.clear();
                                                                t.append(tmp2[l][0]);
                                                                if(xAxis == "diff DN"){
                                                                    t.append(QString::number(tmp2[l][1].toDouble() - tmp2[l2][1].toDouble()));
                                                                    t.append(tmp2[l][2]);
                                                                }else if(yAxis == "diff DN"){
                                                                    t.append(tmp2[l][1]);
                                                                    t.append(QString::number(tmp2[l][2].toDouble() - tmp2[l2][2].toDouble()));

                                                                }
                                                                tmp3.append(t);
                                                                break;
                                                            }
                                                        }
                                                    }
                                                }
                                            }


                                            //それぞれvx、vyに格納
                                            QVector<double> vx;
                                            QVector<double> vy;
                                            if(yAxis == "diff DN" || xAxis == "diff DN"){
                                                for(int l=0; l<tmp3.size(); l++){
                                                    vx.append(tmp3[l][1].toDouble());
                                                    vy.append(tmp3[l][2].toDouble());
                                                }
                                            }else{
                                                for(int l=0; l<tmp2.size(); l++){
                                                    vx.append(tmp2[l][1].toDouble());
                                                    vy.append(tmp2[l][2].toDouble());
                                                }
                                            }


                                            QString s;
                                            //回帰係数を計算し、登録の際に使用するSringを取得
                                            s = getRegressionCoefficientInRegisterRegressionforBlackbody(vx, vy, x-16, y-6);

                                            //取得した登録内容をファイルに書き込み
                                            QFile file(initialFileDirectory+"/" + QString::number(x-16) + "_" + QString::number(y-6) + ".txt");

                                            //ファイルが存在する場合
                                            if(file.exists()){
                                                file.open(QIODevice::WriteOnly);
                                                QTextStream out(&file);

                                                out << s << endl;

                                                file.close();

                                            }else{ //ファイルが存在しなかった場合
                                                //ファイル作成
                                                //QFile ini(QString::number(x-16) + "_" + QString::number(y-6) + "_" + tmp1[l]+ ".txt");
                                                QFile ini(QString::number(x-16) + "_" + QString::number(y-6) + ".txt");

                                                ini.open(QIODevice::WriteOnly);

                                                QTextStream out(&ini);

                                                out << s << endl;

                                                ini.close();

                                                tmp.rename(QString::number(x-16) + "_" + QString::number(y-6) + ".txt",
                                                           initialFileDirectory+ "/" + QString::number(x-16) + "_" + QString::number(y-6) + ".txt");
                                            }



                                            //プログレス表示
                                            p.setLabelText("Registration Progress\n Pixels : " + QString::number(count) + " / " + QString::number(((xmax+1)-xmin)*((ymax+1)-ymin)));
                                            p.setValue(count);
                                            count++;
                                            QCoreApplication::processEvents();

                                            //qDebug() << "(" + QString::number(x-16) + " , " + QString::number(y-6) + ") finish";

                                            query.clear();

                                            //キャンセルボタンが押された場合、関数を終了
                                            if(p.wasCanceled()){

                                                QProgressDialog p;
                                                p.setLabelText("Save Registration Information File Process");
                                                p.setRange(0, 12);
                                                p.show();
                                                QCoreApplication::processEvents();

                                                //保存フォルダパスを取得
                                                //    QFileDialog::Options options = QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly;

                                                //   QString folder = QFileDialog::getExistingDirectory(this, tr("Save Resistration Infomation"),tr("Desktop"), options);
                                                //QString folder = QFileDialog::getExistingDirectory(this,"Register Information save　folder");

                                                //ファイル名が空白なら終了
                                                if(initialFileDirectory==""){
                                                    return;
                                                }

                                                //ファイルオープン
                                                QFile file(initialFileDirectory + "/readme.txt");

                                                //ファイルが存在する場合
                                                if(file.exists()){
                                                    file.open(QIODevice::WriteOnly);
                                                    QTextStream out(&file);

                                                    //データを出力
                                                    out << QString::fromUtf8("作成ファイル") << endl;
                                                    out << endl;
                                                    out << QString::fromUtf8("readme.txt") << endl;
                                                    out << QString::fromUtf8("...このファイル") << endl;
                                                    out << endl;
                                                    out << QString::fromUtf8("a.csv b.csv c.csv d.csv e.csv f.csv g.csv g.csv") << endl;
                                                    out << QString::fromUtf8("...式の形式を DN = g(T+273.15) + h  とし、各係数を３２７×２４７の配列にして表示") << endl;
                                                    out << endl;
                                                    out << QString::fromUtf8("residual.csv") << endl;
                                                    out << QString::fromUtf8("...残差を３２７×２４７の配列にして表示") << endl;
                                                    out << endl;
                                                    out << QString::fromUtf8("0_0.txt ~ 326_246.txt") << endl;
                                                    out << QString::fromUtf8("...係数、各座標の係数、各座標の式") << endl;

                                                    file.close();

                                                    p.setValue(1);
                                                    QCoreApplication::processEvents();

                                                }else{ //ファイルが存在しなかった場合
                                                    //ファイル作成
                                                    QFile ini("readme.txt");

                                                    ini.open(QIODevice::WriteOnly);

                                                    QTextStream out(&ini);
                                                    //データを出力
                                                    out << QString::fromUtf8("作成ファイル") << endl;
                                                    out << endl;
                                                    out << QString::fromUtf8("readme.txt") << endl;
                                                    out << QString::fromUtf8("...このファイル") << endl;
                                                    out << endl;
                                                    out << QString::fromUtf8("a.csv b.csv c.csv d.csv e.csv f.csv g.csv g.csv") << endl;
                                                    out << QString::fromUtf8("...式の形式を DN = g(T+273.15) + h とし、各係数を３２７×２４７の配列にして表示") << endl;
                                                    out << endl;
                                                    out << QString::fromUtf8("residual.csv") << endl;
                                                    out << QString::fromUtf8("...残差を３２７×２４７の配列にして表示") << endl;
                                                    out << endl;
                                                    out << QString::fromUtf8("0_0.txt ~ 326_246.txt") << endl;
                                                    out << QString::fromUtf8("...係数、各座標の係数、各座標の式") << endl;

                                                    ini.close();
                                                    ini.rename("readme.txt", initialFileDirectory + "/readme.txt");

                                                    p.setValue(1);
                                                    QCoreApplication::processEvents();
                                                }

                                                saveFileInRegisterRegression("residual.csv",initialFileDirectory);
                                                p.setValue(2);
                                                QCoreApplication::processEvents();

                                                saveFileInRegisterRegression("a.csv",initialFileDirectory);
                                                p.setValue(3);
                                                QCoreApplication::processEvents();

                                                saveFileInRegisterRegression("a.csv",initialFileDirectory);
                                                p.setValue(4);
                                                QCoreApplication::processEvents();

                                                saveFileInRegisterRegression("b.csv",initialFileDirectory);
                                                p.setValue(5);
                                                QCoreApplication::processEvents();

                                                saveFileInRegisterRegression("c.csv",initialFileDirectory);
                                                p.setValue(6);
                                                QCoreApplication::processEvents();

                                                saveFileInRegisterRegression("d.csv",initialFileDirectory);
                                                p.setValue(7);
                                                QCoreApplication::processEvents();

                                                saveFileInRegisterRegression("e.csv",initialFileDirectory);
                                                p.setValue(8);
                                                QCoreApplication::processEvents();

                                                saveFileInRegisterRegression("f.csv",initialFileDirectory);
                                                p.setValue(9);
                                                QCoreApplication::processEvents();

                                                saveFileInRegisterRegression("g.csv",initialFileDirectory);
                                                p.setValue(10);
                                                QCoreApplication::processEvents();

                                                saveFileInRegisterRegression("h.csv",initialFileDirectory);
                                                p.setValue(11);
                                                QCoreApplication::processEvents();

                                                db.close();
                                                return;
                                            }
                                        }
                                    }


                                    //保存フォルダパスを取得
                                    //  QString initialFileDirectory = QFileDialog::getExistingDirectory(this,"Register Information save　folder");

                                    //ファイル名が空白なら終了
                                    if(initialFileDirectory==""){
                                        return;
                                    }

                                    //ファイルオープン
                                    QFile file(initialFileDirectory + "/readme.txt");

                                    QProgressDialog pp;
                                    pp.setLabelText("Save Registration Information File Process");
                                    pp.setRange(0, 12);
                                    pp.show();
                                    QCoreApplication::processEvents();

                                    //ファイルが存在する場合
                                    if(file.exists()){
                                        file.open(QIODevice::WriteOnly);
                                        QTextStream out(&file);

                                        //データを出力
                                        out << QString::fromUtf8("作成ファイル") << endl;
                                        out << endl;
                                        out << QString::fromUtf8("readme.txt") << endl;
                                        out << QString::fromUtf8("...このファイル") << endl;
                                        out << endl;
                                        out << QString::fromUtf8("a.csv b.csv c.csv d.csv e.csv f.csv g.csv g.csv") << endl;
                                        out << QString::fromUtf8("...式の形式を DN = g(T+273.15) + h  とし、各係数を３２７×２４７の配列にして表示") << endl;
                                        out << endl;
                                        out << QString::fromUtf8("residual.csv") << endl;
                                        out << QString::fromUtf8("...残差を３２７×２４７の配列にして表示") << endl;
                                        out << endl;
                                        out << QString::fromUtf8("0_0.txt ~ 326_246.txt") << endl;
                                        out << QString::fromUtf8("...係数、各座標の係数、各座標の式") << endl;

                                        file.close();

                                        pp.setValue(1);
                                        QCoreApplication::processEvents();

                                    }else{ //ファイルが存在しなかった場合
                                        //ファイル作成
                                        QFile ini("readme.txt");

                                        ini.open(QIODevice::WriteOnly);

                                        QTextStream out(&ini);
                                        //データを出力
                                        out << QString::fromUtf8("作成ファイル") << endl;
                                        out << endl;
                                        out << QString::fromUtf8("readme.txt") << endl;
                                        out << QString::fromUtf8("...このファイル") << endl;
                                        out << endl;
                                        out << QString::fromUtf8("a.csv b.csv c.csv d.csv e.csv f.csv g.csv g.csv") << endl;
                                        out << QString::fromUtf8("...式の形式を DN = g(T+273.15) + h  とし、各係数を３２７×２４７の配列にして表示") << endl;
                                        out << endl;
                                        out << QString::fromUtf8("residual.csv") << endl;
                                        out << QString::fromUtf8("...残差を３２７×２４７の配列にして表示") << endl;
                                        out << endl;
                                        out << QString::fromUtf8("0_0.txt ~ 326_246.txt") << endl;
                                        out << QString::fromUtf8("...係数、各座標の係数、各座標の式") << endl;

                                        ini.close();
                                        ini.rename("readme.txt", initialFileDirectory + "/readme.txt");

                                        pp.setValue(1);
                                        QCoreApplication::processEvents();
                                    }

                                    pp.setValue(2);
                                    QCoreApplication::processEvents();

                                    saveFileInRegisterRegression("residual.csv",initialFileDirectory);
                                    pp.setValue(3);
                                    QCoreApplication::processEvents();

                                    saveFileInRegisterRegression("a.csv",initialFileDirectory);
                                    pp.setValue(4);
                                    QCoreApplication::processEvents();

                                    saveFileInRegisterRegression("a.csv",initialFileDirectory);
                                    pp.setValue(5);
                                    QCoreApplication::processEvents();

                                    saveFileInRegisterRegression("b.csv",initialFileDirectory);
                                    pp.setValue(6);
                                    QCoreApplication::processEvents();

                                    saveFileInRegisterRegression("c.csv",initialFileDirectory);
                                    pp.setValue(7);
                                    QCoreApplication::processEvents();

                                    saveFileInRegisterRegression("d.csv",initialFileDirectory);
                                    pp.setValue(8);
                                    QCoreApplication::processEvents();

                                    saveFileInRegisterRegression("e.csv",initialFileDirectory);
                                    pp.setValue(9);
                                    QCoreApplication::processEvents();

                                    saveFileInRegisterRegression("f.csv",initialFileDirectory);
                                    pp.setValue(10);
                                    QCoreApplication::processEvents();

                                    saveFileInRegisterRegression("g.csv",initialFileDirectory);
                                    pp.setValue(11);
                                    QCoreApplication::processEvents();

                                    saveFileInRegisterRegression("h.csv",initialFileDirectory);
                                    pp.setValue(12);
                                    QCoreApplication::processEvents();

                                    //グラフを全部、赤色に設定
                                    for(int i=1; i<regressionGraphCount; i++){
                                        ui->widget->graph(i)->setPen(QPen(Qt::red));
                                    }
                                    ui->widget->selectedGraphs().first()->setPen(QPen(Qt::darkGreen)); //登録した（選択されている）グラフを緑色に変更

                                    db.close();

                                    QMessageBox msgBox2;
                                    msgBox2.setText("This formula registration have been complete.");
                                    msgBox2.setStandardButtons(QMessageBox::Ok);
                                    msgBox2.exec();
                                }

                                */

QString CalibrationGraph::getRegressionCoefficientInRegisterRegression(
    QVector<double> vx, QVector<double> vy, int xc, int yc) {

    int num;
    double x = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0, x6 = 0, x7 = 0, x8 = 0, x9 = 0, 
    x10 = 0, x11 = 0, x12 = 0, x13 = 0, x14 = 0;
    double x7y = 0, x6y = 0, x5y = 0, x4y = 0, x3y = 0, x2y = 0, xy = 0, y = 0;
    double pivot, mul;
    double aa, bb, cc, dd, ee, ff, gg, hh;
    QString coefficient, formula;

    //次数の取得 & +1の処理
    num = ui->widget->selectedGraphs().at(0)->name().section(",", 0, 0).toInt() + 1;

    //qDebug() << num;

    //係数の初期化
    aa = bb = cc = dd = ee = ff = gg = hh = 0;

    //それぞれの総和

    for (int i = 0; i < vx.size(); i++)
    {
        x += vx[i];
        x2 += pow(vx[i], 2);
        x3 += pow(vx[i], 3);
        x4 += pow(vx[i], 4);
        x5 += pow(vx[i], 5);
        x6 += pow(vx[i], 6);
        x7 += pow(vx[i], 7);
        x8 += pow(vx[i], 8);
        x9 += pow(vx[i], 9);
        x10 += pow(vx[i], 10);
        x11 += pow(vx[i], 11);
        x12 += pow(vx[i], 12);
        x13 += pow(vx[i], 13);
        x14 += pow(vx[i], 14);
        x7y += (pow(vx[i], 7) * vy[i]);
        x6y += (pow(vx[i], 6) * vy[i]);
        x5y += (pow(vx[i], 5) * vy[i]);
        x4y += (pow(vx[i], 4) * vy[i]);
        x3y += (pow(vx[i], 3) * vy[i]);
        x2y += (pow(vx[i], 2) * vy[i]);
        xy += (vx[i] * vy[i]);
        y += vy[i];
    }

    // 拡大係数行列 M
    double M[8][8 + 1] =
    {
        {x14, x13, x12, x11, x10, x9, x8, x7, x7y},
        {x13, x12, x11, x10, x9, x8, x7, x6, x6y},
        {x12, x11, x10, x9, x8, x7, x6, x5, x5y},
        {x11, x10, x9, x8, x7, x6, x5, x4, x4y},
        {x10, x9, x8, x7, x6, x5, x4, x3, x3y},
        {x9, x8, x7, x6, x5, x4, x3, x2, x2y},
        {x8, x7, x6, x5, x4, x3, x2, x, xy},
        {x7, x6, x5, x4, x3, x2, x, double(vx.size()), y}};

    double **M2 = new double *[num];
    for (int i = 0; i < num; i++)
    {
        M2[i] = new double[num + 1];
    }

    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < num + 1; j++)
        {
            M2[i][j] = M[(8 - num) + i][(8 - num) + j];
        }
    }

    // 対角成分が1で正規化された階段行列を作る(前進消去)
    for (int i = 0; i < num; ++i)
    {
        // 対角成分の選択、この値で行成分を正規化
        pivot = M2[i][i];
        for (int j = 0; j < num + 1; ++j)
        {
            M2[i][j] = (1 / pivot) * M2[i][j];
        }

        // 階段行列を作る為に、現在の行より下の行について
        // i列目の成分が0になるような基本変形をする
        for (int k = i + 1; k < num; ++k)
        {
            mul = M2[k][i];
            for (int count = i; count < num + 1; ++count)
            {
                M2[k][count] = M2[k][count] - mul * M2[i][count];
            }
        }
    }

    // 下から上に向かって変数に代入して、独立した解の形にする(後進代入)
    // このとき一番下の行はすでに独立した解を得ている
    for (int i = num - 1; i > 0; --i)
    {
        for (int k = i - 1; k >= 0; --k)
        {
            mul = M2[k][i];
            for (int count = i; count < num + 1; ++count)
            {
                M2[k][count] = M2[k][count] - mul * M2[i][count];
            }
        }
    }

    //対応する係数を格納
    if (1 < num)
    {
        hh = M2[num - 1][num];
        gg = M2[num - 2][num];
    }
    if (2 < num)
    {
        ff = M2[num - 3][num];
    }
    if (3 < num)
    {
        ee = M2[num - 4][num];
    }
    if (4 < num)
    {
        dd = M2[num - 5][num];
    }
    if (5 < num)
    {
        cc = M2[num - 6][num];
    }
    if (6 < num)
    {
        bb = M2[num - 7][num];
    }
    if (7 < num)
    {
        aa = M2[num - 8][num];
    }

    //メモリの解放
    for (int i = 0; i < num; i++)
    {
        delete[] M2[i];
    }
    delete[] M2;

    //残差を求める
    double d = 0;
    double tmp;
    for (int i = 0; i < vx.size(); i++)
    {
        tmp = vy[i] - (aa * pow(vx[i], 7) + bb * pow(vx[i], 6) + cc * pow(vx[i], 5) + dd * pow(vx[i], 4) + ee * pow(vx[i], 3) + ff * pow(vx[i], 2) + gg * vx[i] + hh);
        if (tmp < 0)
        {
            d += tmp * -1;
        }
        else
        {
            d += tmp;
        }
    }

    d = d / vx.size();
    //ファイルに記述する際に使用する文を作成

    residual[yc][xc] = QString::number(d);
    arra[yc][xc] = QString::number(aa);
    arrb[yc][xc] = QString::number(bb);
    arrc[yc][xc] = QString::number(cc);
    arrd[yc][xc] = QString::number(dd);
    arre[yc][xc] = QString::number(ee);
    arrf[yc][xc] = QString::number(ff);
    arrg[yc][xc] = QString::number(gg);
    arrh[yc][xc] = QString::number(hh);

    //ファイルに記述する際に使用する文を作成

    coefficient.append(QString::number(aa) + ",");
    coefficient.append(QString::number(bb) + ",");
    coefficient.append(QString::number(cc) + ",");
    coefficient.append(QString::number(dd) + ",");
    coefficient.append(QString::number(ee) + ",");
    coefficient.append(QString::number(ff) + ",");
    coefficient.append(QString::number(gg) + ",");
    coefficient.append(QString::number(hh) + ",");

    num = num - 1;

    if (7 <= num)
    {
        formula.append(QString::number(aa) + "x^7 ");
    }

    if (6 == num)
    {
        formula.append(QString::number(bb) + "x^6 ");
    }
    else if (6 <= num)
    {
        if (bb < 0)
        {
            formula.append("- " + QString::number(-1 * bb) + "x^6 ");
        }
        else
        {
            formula.append("+ " + QString::number(bb) + "x^6 ");
        }
    }

    if (5 == num)
    {
        formula.append(QString::number(cc) + "x^5 ");
    }
    else if (5 <= num)
    {
        if (cc < 0)
        {
            formula.append("- " + QString::number(-1 * cc) + "x^5 ");
        }
        else
        {
            formula.append("+ " + QString::number(cc) + "x^5 ");
        }
    }

    if (4 == num)
    {
        formula.append(QString::number(dd) + "x^4 ");
    }
    else if (4 <= num)
    {
        if (dd < 0)
        {
            formula.append("- " + QString::number(-1 * dd) + "x^4 ");
        }
        else
        {
            formula.append("+ " + QString::number(dd) + "x^4 ");
        }
    }

    if (3 == num)
    {
        formula.append(QString::number(ee) + "x^3 ");
    }
    else if (3 <= num)
    {
        if (ee < 0)
        {
            formula.append("- " + QString::number(-1 * ee) + "x^3 ");
        }
        else
        {
            formula.append("+ " + QString::number(ee) + "x^3 ");
        }
    }

    if (2 == num)
    {
        formula.append(QString::number(ff) + "x^2 ");
    }
    else if (2 <= num)
    {
        if (ff < 0)
        {
            formula.append("- " + QString::number(-1 * ff) + "x^2 ");
        }
        else
        {
            formula.append("+ " + QString::number(ff) + "x^2 ");
        }
    }

    if (1 == num)
    {
        formula.append(QString::number(gg) + "x ");
    }
    else if (1 <= num)
    {
        if (gg < 0)
        {
            formula.append("- " + QString::number(-1 * gg) + "x ");
        }
        else
        {
            formula.append("+ " + QString::number(gg) + "x ");
        }
    }

    if (hh < 0)
    {
        formula.append("- " + QString::number(-1 * hh));
    }
    else
    {
        formula.append("+ " + QString::number(hh));
    }

    return QString::number(num) + "," + coefficient + formula;
}

QString CalibrationGraph::getRegressionCoefficientInRegisterRegressionforBlackbody(QVector<double> vx, QVector<double> vy, int xc, int yc)
{

    int num;
    double x = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0, x6 = 0, x7 = 0, x8 = 0, x9 = 0, x10 = 0, x11 = 0, x12 = 0, x13 = 0, x14 = 0;
    double x7y = 0, x6y = 0, x5y = 0, x4y = 0, x3y = 0, x2y = 0, xy = 0, y = 0;
    double pivot, mul;
    double aa, bb, cc, dd, ee, ff, gg, hh;
    QString coefficient, formula;
    QString num2 = ui->degreeComboBox->currentText();
    //次数の取得 & +1の処理
    //num = ui->widget->selectedGraphs().at(0)->name().section(",",0,0).toInt() + 1;
    num = 2;
    //qDebug() << num;

    //係数の初期化
    aa = bb = cc = dd = ee = ff = gg = hh = 0;

    //それぞれの総和
    for (int i = 0; i < vx.size(); i++)
    {
        x += planck(vx[i] + 273.15);
        x2 += pow(planck(vx[i] + 273.15), 2);
        x3 += pow(planck(vx[i] + 273.15), 3);
        x4 += pow(planck(vx[i] + 273.15), 4);
        x5 += pow(planck(vx[i] + 273.15), 5);
        x6 += pow(planck(vx[i] + 273.15), 6);
        x7 += pow(planck(vx[i] + 273.15), 7);
        x8 += pow(planck(vx[i] + 273.15), 8);
        x9 += pow(planck(vx[i] + 273.15), 9);
        x10 += pow(planck(vx[i] + 273.15), 10);
        x11 += pow(planck(vx[i] + 273.15), 11);
        x12 += pow(planck(vx[i] + 273.15), 12);
        x13 += pow(planck(vx[i] + 273.15), 13);
        x14 += pow(planck(vx[i] + 273.15), 14);
        x7y += (pow(planck(vx[i] + 273.15), 7) * vy[i]);
        x6y += (pow(planck(vx[i] + 273.15), 6) * vy[i]);
        x5y += (pow(planck(vx[i] + 273.15), 5) * vy[i]);
        x4y += (pow(planck(vx[i] + 273.15), 4) * vy[i]);
        x3y += (pow(planck(vx[i] + 273.15), 3) * vy[i]);
        x2y += (pow(planck(vx[i] + 273.15), 2) * vy[i]);
        xy += (planck(vx[i] + 273.15) * vy[i]);
        y += vy[i];
        /*   cout<<"T"<<endl;
                                     cout<<vx[i]<<endl;

                                        cout<<"F"<<endl;
                                        cout<<planck(vx[i]+273.15)<<endl;
                                */
    }

    // 拡大係数行列 M
    double M[8][8 + 1] =
    {
        {x14, x13, x12, x11, x10, x9, x8, x7, x7y},
        {x13, x12, x11, x10, x9, x8, x7, x6, x6y},
        {x12, x11, x10, x9, x8, x7, x6, x5, x5y},
        {x11, x10, x9, x8, x7, x6, x5, x4, x4y},
        {x10, x9, x8, x7, x6, x5, x4, x3, x3y},
        {x9, x8, x7, x6, x5, x4, x3, x2, x2y},
        {x8, x7, x6, x5, x4, x3, x2, x, xy},
        {x7, x6, x5, x4, x3, x2, x, double(vx.size()), y}};

    double **M2 = new double *[num];
    for (int i = 0; i < num; i++)
    {
        M2[i] = new double[num + 1];
    }

    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < num + 1; j++)
        {
            M2[i][j] = M[(8 - num) + i][(8 - num) + j];
        }
    }

    // 対角成分が1で正規化された階段行列を作る(前進消去)
    for (int i = 0; i < num; ++i)
    {
        // 対角成分の選択、この値で行成分を正規化
        pivot = M2[i][i];
        for (int j = 0; j < num + 1; ++j)
        {
            M2[i][j] = (1 / pivot) * M2[i][j];
        }

        // 階段行列を作る為に、現在の行より下の行について
        // i列目の成分が0になるような基本変形をする
        for (int k = i + 1; k < num; ++k)
        {
            mul = M2[k][i];
            for (int count = i; count < num + 1; ++count)
            {
                M2[k][count] = M2[k][count] - mul * M2[i][count];
            }
        }
    }

    // 下から上に向かって変数に代入して、独立した解の形にする(後進代入)
    // このとき一番下の行はすでに独立した解を得ている
    for (int i = num - 1; i > 0; --i)
    {
        for (int k = i - 1; k >= 0; --k)
        {
            mul = M2[k][i];
            for (int count = i; count < num + 1; ++count)
            {
                M2[k][count] = M2[k][count] - mul * M2[i][count];
            }
        }
    }

    //対応する係数を格納
    if (1 < num)
    {
        hh = M2[num - 1][num];
        gg = M2[num - 2][num];
    }
    if (2 < num)
    {
        ff = M2[num - 3][num];
    }
    if (3 < num)
    {
        ee = M2[num - 4][num];
    }
    if (4 < num)
    {
        dd = M2[num - 5][num];
    }
    if (5 < num)
    {
        cc = M2[num - 6][num];
    }
    if (6 < num)
    {
        bb = M2[num - 7][num];
    }
    if (7 < num)
    {
        aa = M2[num - 8][num];
    }

    //メモリの解放
    for (int i = 0; i < num; i++)
    {
        delete[] M2[i];
    }
    delete[] M2;

    //残差を求める
    double d = 0;
    double tmp;
    for (int i = 0; i < vx.size(); i++)
    {
        tmp = vy[i] - (gg * vx[i] + hh);
        if (tmp < 0)
        {
            d += tmp * -1;
        }
        else
        {
            d += tmp;
        }
    }
    d = d / vx.size();
    //残差配列に座標に対応する残差を格納
    residual[yc][xc] = QString::number(d);
    arra[yc][xc] = QString::number(aa);
    arrb[yc][xc] = QString::number(bb);
    arrc[yc][xc] = QString::number(cc);
    arrd[yc][xc] = QString::number(dd);
    arre[yc][xc] = QString::number(ee);
    arrf[yc][xc] = QString::number(ff);
    arrg[yc][xc] = QString::number(gg);
    arrh[yc][xc] = QString::number(hh);

    //ファイルに記述する際に使用する文を作成

    coefficient.append(QString::number(aa) + ",");
    coefficient.append(QString::number(bb) + ",");
    coefficient.append(QString::number(cc) + ",");
    coefficient.append(QString::number(dd) + ",");
    coefficient.append(QString::number(ee) + ",");
    coefficient.append(QString::number(ff) + ",");
    coefficient.append(QString::number(gg) + ",");
    coefficient.append(QString::number(hh) + ",");

    num = num - 1;

    if (7 <= num)
    {
        formula.append(QString::number(aa) + "x^7 ");
    }

    if (6 == num)
    {
        formula.append(QString::number(bb) + "x^6 ");
    }
    else if (6 <= num)
    {
        if (bb < 0)
        {
            formula.append("- " + QString::number(-1 * bb) + "x^6 ");
        }
        else
        {
            formula.append("+ " + QString::number(bb) + "x^6 ");
        }
    }

    if (5 == num)
    {
        formula.append(QString::number(cc) + "x^5 ");
    }
    else if (5 <= num)
    {
        if (cc < 0)
        {
            formula.append("- " + QString::number(-1 * cc) + "x^5 ");
        }
        else
        {
            formula.append("+ " + QString::number(cc) + "x^5 ");
        }
    }

    if (4 == num)
    {
        formula.append(QString::number(dd) + "x^4 ");
    }
    else if (4 <= num)
    {
        if (dd < 0)
        {
            formula.append("- " + QString::number(-1 * dd) + "x^4 ");
        }
        else
        {
            formula.append("+ " + QString::number(dd) + "x^4 ");
        }
    }

    if (3 == num)
    {
        formula.append(QString::number(ee) + "x^3 ");
    }
    else if (3 <= num)
    {
        if (ee < 0)
        {
            formula.append("- " + QString::number(-1 * ee) + "x^3 ");
        }
        else
        {
            formula.append("+ " + QString::number(ee) + "x^3 ");
        }
    }

    if (2 == num)
    {
        formula.append(QString::number(ff) + "x^2 ");
    }
    else if (2 <= num)
    {
        if (ff < 0)
        {
            formula.append("- " + QString::number(-1 * ff) + "x^2 ");
        }
        else
        {
            formula.append("+ " + QString::number(ff) + "x^2 ");
        }
    }

    if (1 == num || num2 == "Black_Body")
    {
        if (num2 == "Black_Body")
            formula.append(QString::number(gg) + "*F( T + 273.15) ");
        else
            formula.append(QString::number(gg) + "x ");
    }
    else if (1 <= num)
    {
        if (gg < 0)
        {
            if (num2 == "Black_Body")
                formula.append("- " + QString::number(-1 * gg) + "*F( T + 273.15) ");
            else
                formula.append("- " + QString::number(-1 * gg) + "x ");
        }
        else
        {
            if (num2 == "Black_Body")
                formula.append("+ " + QString::number(gg) + "*F( T + 273.15) ");
            else
                formula.append("+ " + QString::number(gg) + "x ");
        }
    }

    if (hh < 0)
    {
        formula.append("- " + QString::number(-1 * hh));
    }
    else
    {
        formula.append("+ " + QString::number(hh));
    }

    return QString::number(num) + "," + coefficient + formula;
}

//データベース接続
void CalibrationGraph::connectDB()
{
    db.open();
    query = QSqlQuery(db);
    if (query.isActive())
    {
        query.first();
    }
}

//検索すべきテーブルの名前の判断
QString CalibrationGraph::judgeTableName(int x, int y)
{
    if (0 <= y && y <= 31)
    {
        if (0 <= x && x <= 31)
            return "pix01";
        else if (32 <= x && x <= 63)
            return "pix02";
        else if (64 <= x && x <= 95)
            return "pix03";
        else if (96 <= x && x <= 127)
            return "pix04";
        else if (128 <= x && x <= 159)
            return "pix05";
        else if (160 <= x && x <= 191)
            return "pix06";
        else if (192 <= x && x <= 223)
            return "pix07";
        else if (224 <= x && x <= 255)
            return "pix08";
        else if (256 <= x && x <= 287)
            return "pix09";
        else if (288 <= x && x <= 319)
            return "pix10";
        else if (320 <= x && x <= 351)
            return "pix11";
        else if (352 <= x && x <= 383)
            return "pix12";
    }
    else if (32 <= y && y <= 63)
    {
        if (0 <= x && x <= 31)
            return "pix13";
        else if (32 <= x && x <= 63)
            return "pix14";
        else if (64 <= x && x <= 95)
            return "pix15";
        else if (96 <= x && x <= 127)
            return "pix16";
        else if (128 <= x && x <= 159)
            return "pix17";
        else if (160 <= x && x <= 191)
            return "pix18";
        else if (192 <= x && x <= 223)
            return "pix19";
        else if (224 <= x && x <= 255)
            return "pix20";
        else if (256 <= x && x <= 287)
            return "pix21";
        else if (288 <= x && x <= 319)
            return "pix22";
        else if (320 <= x && x <= 351)
            return "pix23";
        else if (352 <= x && x <= 383)
            return "pix24";
    }
    else if (64 <= y && y <= 95)
    {
        if (0 <= x && x <= 31)
            return "pix25";
        else if (32 <= x && x <= 63)
            return "pix26";
        else if (64 <= x && x <= 95)
            return "pix27";
        else if (96 <= x && x <= 127)
            return "pix28";
        else if (128 <= x && x <= 159)
            return "pix29";
        else if (160 <= x && x <= 191)
            return "pix30";
        else if (192 <= x && x <= 223)
            return "pix31";
        else if (224 <= x && x <= 255)
            return "pix32";
        else if (256 <= x && x <= 287)
            return "pix33";
        else if (288 <= x && x <= 319)
            return "pix34";
        else if (320 <= x && x <= 351)
            return "pix35";
        else if (352 <= x && x <= 383)
            return "pix36";
    }
    else if (96 <= y && y <= 127)
    {
        if (0 <= x && x <= 31)
            return "pix37";
        else if (32 <= x && x <= 63)
            return "pix38";
        else if (64 <= x && x <= 95)
            return "pix39";
        else if (96 <= x && x <= 127)
            return "pix40";
        else if (128 <= x && x <= 159)
            return "pix41";
        else if (160 <= x && x <= 191)
            return "pix42";
        else if (192 <= x && x <= 223)
            return "pix43";
        else if (224 <= x && x <= 255)
            return "pix44";
        else if (256 <= x && x <= 287)
            return "pix45";
        else if (288 <= x && x <= 319)
            return "pix46";
        else if (320 <= x && x <= 351)
            return "pix47";
        else if (352 <= x && x <= 383)
            return "pix48";
    }
    else if (128 <= y && y <= 159)
    {
        if (0 <= x && x <= 31)
            return "pix49";
        else if (32 <= x && x <= 63)
            return "pix50";
        else if (64 <= x && x <= 95)
            return "pix51";
        else if (96 <= x && x <= 127)
            return "pix52";
        else if (128 <= x && x <= 159)
            return "pix53";
        else if (160 <= x && x <= 191)
            return "pix54";
        else if (192 <= x && x <= 223)
            return "pix55";
        else if (224 <= x && x <= 255)
            return "pix56";
        else if (256 <= x && x <= 287)
            return "pix57";
        else if (288 <= x && x <= 319)
            return "pix58";
        else if (320 <= x && x <= 351)
            return "pix59";
        else if (352 <= x && x <= 383)
            return "pix60";
    }
    else if (160 <= y && y <= 191)
    {
        if (0 <= x && x <= 31)
            return "pix61";
        else if (32 <= x && x <= 63)
            return "pix62";
        else if (64 <= x && x <= 95)
            return "pix63";
        else if (96 <= x && x <= 127)
            return "pix64";
        else if (128 <= x && x <= 159)
            return "pix65";
        else if (160 <= x && x <= 191)
            return "pix66";
        else if (192 <= x && x <= 223)
            return "pix67";
        else if (224 <= x && x <= 255)
            return "pix68";
        else if (256 <= x && x <= 287)
            return "pix69";
        else if (288 <= x && x <= 319)
            return "pix70";
        else if (320 <= x && x <= 351)
            return "pix71";
        else if (352 <= x && x <= 383)
            return "pix72";
    }
    else if (192 <= y && y <= 223)
    {
        if (0 <= x && x <= 31)
            return "pix73";
        else if (32 <= x && x <= 63)
            return "pix74";
        else if (64 <= x && x <= 95)
            return "pix75";
        else if (96 <= x && x <= 127)
            return "pix76";
        else if (128 <= x && x <= 159)
            return "pix77";
        else if (160 <= x && x <= 191)
            return "pix78";
        else if (192 <= x && x <= 223)
            return "pix79";
        else if (224 <= x && x <= 255)
            return "pix80";
        else if (256 <= x && x <= 287)
            return "pix81";
        else if (288 <= x && x <= 319)
            return "pix82";
        else if (320 <= x && x <= 351)
            return "pix83";
        else if (352 <= x && x <= 383)
            return "pix84";
    }
    else if (224 <= y && y <= 255)
    {
        if (0 <= x && x <= 31)
            return "pix85";
        else if (32 <= x && x <= 63)
            return "pix86";
        else if (64 <= x && x <= 95)
            return "pix87";
        else if (96 <= x && x <= 127)
            return "pix88";
        else if (128 <= x && x <= 159)
            return "pix89";
        else if (160 <= x && x <= 191)
            return "pix90";
        else if (192 <= x && x <= 223)
            return "pix91";
        else if (224 <= x && x <= 255)
            return "pix92";
        else if (256 <= x && x <= 287)
            return "pix93";
        else if (288 <= x && x <= 319)
            return "pix94";
        else if (320 <= x && x <= 351)
            return "pix95";
        else if (352 <= x && x <= 383)
            return "pix96";
    }

    return "";
}

int CalibrationGraph::judgeTableNameint(int x, int y)
{
    if (0 <= y && y <= 31)
    {
        if (0 <= x && x <= 31)
            return 1;
        else if (32 <= x && x <= 63)
            return 2;
        else if (64 <= x && x <= 95)
            return 3;
        else if (96 <= x && x <= 127)
            return 4;
        else if (128 <= x && x <= 159)
            return 5;
        else if (160 <= x && x <= 191)
            return 6;
        else if (192 <= x && x <= 223)
            return 7;
        else if (224 <= x && x <= 255)
            return 8;
        else if (256 <= x && x <= 287)
            return 9;
        else if (288 <= x && x <= 319)
            return 10;
        else if (320 <= x && x <= 351)
            return 11;
        else if (352 <= x && x <= 383)
            return 12;
    }
    else if (32 <= y && y <= 63)
    {
        if (0 <= x && x <= 31)
            return 13;
        else if (32 <= x && x <= 63)
            return 14;
        else if (64 <= x && x <= 95)
            return 15;
        else if (96 <= x && x <= 127)
            return 16;
        else if (128 <= x && x <= 159)
            return 17;
        else if (160 <= x && x <= 191)
            return 18;
        else if (192 <= x && x <= 223)
            return 19;
        else if (224 <= x && x <= 255)
            return 20;
        else if (256 <= x && x <= 287)
            return 21;
        else if (288 <= x && x <= 319)
            return 22;
        else if (320 <= x && x <= 351)
            return 23;
        else if (352 <= x && x <= 383)
            return 24;
    }
    else if (64 <= y && y <= 95)
    {
        if (0 <= x && x <= 31)
            return 25;
        else if (32 <= x && x <= 63)
            return 26;
        else if (64 <= x && x <= 95)
            return 27;
        else if (96 <= x && x <= 127)
            return 28;
        else if (128 <= x && x <= 159)
            return 29;
        else if (160 <= x && x <= 191)
            return 30;
        else if (192 <= x && x <= 223)
            return 31;
        else if (224 <= x && x <= 255)
            return 32;
        else if (256 <= x && x <= 287)
            return 33;
        else if (288 <= x && x <= 319)
            return 34;
        else if (320 <= x && x <= 351)
            return 35;
        else if (352 <= x && x <= 383)
            return 36;
    }
    else if (96 <= y && y <= 127)
    {
        if (0 <= x && x <= 31)
            return 37;
        else if (32 <= x && x <= 63)
            return 38;
        else if (64 <= x && x <= 95)
            return 39;
        else if (96 <= x && x <= 127)
            return 40;
        else if (128 <= x && x <= 159)
            return 41;
        else if (160 <= x && x <= 191)
            return 42;
        else if (192 <= x && x <= 223)
            return 43;
        else if (224 <= x && x <= 255)
            return 44;
        else if (256 <= x && x <= 287)
            return 45;
        else if (288 <= x && x <= 319)
            return 46;
        else if (320 <= x && x <= 351)
            return 47;
        else if (352 <= x && x <= 383)
            return 48;
    }
    else if (128 <= y && y <= 159)
    {
        if (0 <= x && x <= 31)
            return 49;
        else if (32 <= x && x <= 63)
            return 50;
        else if (64 <= x && x <= 95)
            return 51;
        else if (96 <= x && x <= 127)
            return 52;
        else if (128 <= x && x <= 159)
            return 53;
        else if (160 <= x && x <= 191)
            return 54;
        else if (192 <= x && x <= 223)
            return 55;
        else if (224 <= x && x <= 255)
            return 56;
        else if (256 <= x && x <= 287)
            return 57;
        else if (288 <= x && x <= 319)
            return 58;
        else if (320 <= x && x <= 351)
            return 59;
        else if (352 <= x && x <= 383)
            return 60;
    }
    else if (160 <= y && y <= 191)
    {
        if (0 <= x && x <= 31)
            return 61;
        else if (32 <= x && x <= 63)
            return 62;
        else if (64 <= x && x <= 95)
            return 63;
        else if (96 <= x && x <= 127)
            return 64;
        else if (128 <= x && x <= 159)
            return 65;
        else if (160 <= x && x <= 191)
            return 66;
        else if (192 <= x && x <= 223)
            return 67;
        else if (224 <= x && x <= 255)
            return 68;
        else if (256 <= x && x <= 287)
            return 69;
        else if (288 <= x && x <= 319)
            return 70;
        else if (320 <= x && x <= 351)
            return 71;
        else if (352 <= x && x <= 383)
            return 72;
    }
    else if (192 <= y && y <= 223)
    {
        if (0 <= x && x <= 31)
            return 73;
        else if (32 <= x && x <= 63)
            return 74;
        else if (64 <= x && x <= 95)
            return 75;
        else if (96 <= x && x <= 127)
            return 76;
        else if (128 <= x && x <= 159)
            return 77;
        else if (160 <= x && x <= 191)
            return 78;
        else if (192 <= x && x <= 223)
            return 79;
        else if (224 <= x && x <= 255)
            return 80;
        else if (256 <= x && x <= 287)
            return 81;
        else if (288 <= x && x <= 319)
            return 82;
        else if (320 <= x && x <= 351)
            return 83;
        else if (352 <= x && x <= 383)
            return 84;
    }
    else if (224 <= y && y <= 255)
    {
        if (0 <= x && x <= 31)
            return 85;
        else if (32 <= x && x <= 63)
            return 86;
        else if (64 <= x && x <= 95)
            return 87;
        else if (96 <= x && x <= 127)
            return 88;
        else if (128 <= x && x <= 159)
            return 89;
        else if (160 <= x && x <= 191)
            return 90;
        else if (192 <= x && x <= 223)
            return 91;
        else if (224 <= x && x <= 255)
            return 92;
        else if (256 <= x && x <= 287)
            return 93;
        else if (288 <= x && x <= 319)
            return 94;
        else if (320 <= x && x <= 351)
            return 95;
        else if (352 <= x && x <= 383)
            return 96;
    }
}


void CalibrationGraph::getDataPath(QString path)
{
    databPath = path;
    // qDebug() << databPath;
}

void CalibrationGraph::saveFileInRegisterRegression(QString fileName, QString folder)
{

    //ファイルオープン aaaaa
    QFile file(folder + "/" + fileName);

    //ファイルが存在する場合
    if (file.exists())
    {
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);

        //データを出力
        for (int k = 0; k < 248; k++)
        {
            for (int m = 0; m < 328; m++)
            {
                if (fileName == "residual.csv")
                {
                    out << residual[k][m] << ",";
                }
                else if (fileName == "a.csv")
                {
                    out << arra[k][m] << ",";
                }
                else if (fileName == "b.csv")
                {
                    out << arrb[k][m] << ",";
                }
                else if (fileName == "c.csv")
                {
                    out << arrc[k][m] << ",";
                }
                else if (fileName == "d.csv")
                {
                    out << arrd[k][m] << ",";
                }
                else if (fileName == "e.csv")
                {
                    out << arre[k][m] << ",";
                }
                else if (fileName == "f.csv")
                {
                    out << arrf[k][m] << ",";
                }
                else if (fileName == "g.csv")
                {
                    out << arrg[k][m] << ",";
                }
                else
                {
                    out << arrh[k][m] << ",";
                }
            }
            out << endl;
        }

        file.close();
    }
    else
    { //ファイルが存在しなかった場合
        //ファイル作成
        QFile ini(fileName);

        ini.open(QIODevice::WriteOnly);

        QTextStream out(&ini);

        for (int k = 0; k < 248; k++)
        {
            for (int m = 0; m < 328; m++)
            {
                if (fileName == "residual.csv")
                {
                    out << residual[k][m] << ",";
                }
                else if (fileName == "a.csv")
                {
                    out << arra[k][m] << ",";
                }
                else if (fileName == "b.csv")
                {
                    out << arrb[k][m] << ",";
                }
                else if (fileName == "c.csv")
                {
                    out << arrc[k][m] << ",";
                }
                else if (fileName == "d.csv")
                {
                    out << arrd[k][m] << ",";
                }
                else if (fileName == "e.csv")
                {
                    out << arre[k][m] << ",";
                }
                else if (fileName == "f.csv")
                {
                    out << arrf[k][m] << ",";
                }
                else if (fileName == "g.csv")
                {
                    out << arrg[k][m] << ",";
                }
                else
                {
                    out << arrh[k][m] << ",";
                }
            }
            out << endl;
        }

        ini.close();

        ini.rename(fileName, folder + "/" + fileName);
    }
}

void ForThread1::saveFileInRegisterRegression(QString fileName, QString folder)
{

    //ファイルオープン
    QFile file(folder + "/" + fileName);

    //ファイルが存在する場合
    if (file.exists())
    {
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);

        //データを出力
        for (int k = 0; k < 248; k++)
        {
            for (int m = 0; m < 328; m++)
            {
                if (fileName == "residual.csv")
                {
                    out << residual[k][m] << ",";
                }
                else if (fileName == "a.csv")
                {
                    out << arra[k][m] << ",";
                }
                else if (fileName == "b.csv")
                {
                    out << arrb[k][m] << ",";
                }
                else if (fileName == "c.csv")
                {
                    out << arrc[k][m] << ",";
                }
                else if (fileName == "d.csv")
                {
                    out << arrd[k][m] << ",";
                }
                else if (fileName == "e.csv")
                {
                    out << arre[k][m] << ",";
                }
                else if (fileName == "f.csv")
                {
                    out << arrf[k][m] << ",";
                }
                else if (fileName == "g.csv")
                {
                    out << arrg[k][m] << ",";
                }
                else
                {
                    out << arrh[k][m] << ",";
                }
            }
            out << endl;
        }

        file.close();
    }
    else
    { //ファイルが存在しなかった場合
        //ファイル作成
        QFile ini(fileName);

        ini.open(QIODevice::WriteOnly);

        QTextStream out(&ini);

        for (int k = 0; k < 248; k++)
        {
            for (int m = 0; m < 328; m++)
            {
                if (fileName == "residual.csv")
                {
                    out << residual[k][m] << ",";
                }
                else if (fileName == "a.csv")
                {
                    out << arra[k][m] << ",";
                }
                else if (fileName == "b.csv")
                {
                    out << arrb[k][m] << ",";
                }
                else if (fileName == "c.csv")
                {
                    out << arrc[k][m] << ",";
                }
                else if (fileName == "d.csv")
                {
                    out << arrd[k][m] << ",";
                }
                else if (fileName == "e.csv")
                {
                    out << arre[k][m] << ",";
                }
                else if (fileName == "f.csv")
                {
                    out << arrf[k][m] << ",";
                }
                else if (fileName == "g.csv")
                {
                    out << arrg[k][m] << ",";
                }
                else
                {
                    out << arrh[k][m] << ",";
                }
            }
            out << endl;
        }

        ini.close();

        ini.rename(fileName, folder + "/" + fileName);
    }
}

void ForThread2::saveFileInRegisterRegression(QString fileName, QString folder)
{

    //ファイルオープン
    QFile file(folder + "/" + fileName);

    //ファイルが存在する場合
    if (file.exists())
    {
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);

        //データを出力
        for (int k = 0; k < 248; k++)
        {
            for (int m = 0; m < 328; m++)
            {
                if (fileName == "residual.csv")
                {
                    out << residual[k][m] << ",";
                }
                else if (fileName == "a.csv")
                {
                    out << arra[k][m] << ",";
                }
                else if (fileName == "b.csv")
                {
                    out << arrb[k][m] << ",";
                }
                else if (fileName == "c.csv")
                {
                    out << arrc[k][m] << ",";
                }
                else if (fileName == "d.csv")
                {
                    out << arrd[k][m] << ",";
                }
                else if (fileName == "e.csv")
                {
                    out << arre[k][m] << ",";
                }
                else if (fileName == "f.csv")
                {
                    out << arrf[k][m] << ",";
                }
                else if (fileName == "g.csv")
                {
                    out << arrg[k][m] << ",";
                }
                else
                {
                    out << arrh[k][m] << ",";
                }
            }
            out << endl;
        }

        file.close();
    }
    else
    { //ファイルが存在しなかった場合
        //ファイル作成
        QFile ini(fileName);

        ini.open(QIODevice::WriteOnly);

        QTextStream out(&ini);

        for (int k = 0; k < 248; k++)
        {
            for (int m = 0; m < 328; m++)
            {
                if (fileName == "residual.csv")
                {
                    out << residual[k][m] << ",";
                }
                else if (fileName == "a.csv")
                {
                    out << arra[k][m] << ",";
                }
                else if (fileName == "b.csv")
                {
                    out << arrb[k][m] << ",";
                }
                else if (fileName == "c.csv")
                {
                    out << arrc[k][m] << ",";
                }
                else if (fileName == "d.csv")
                {
                    out << arrd[k][m] << ",";
                }
                else if (fileName == "e.csv")
                {
                    out << arre[k][m] << ",";
                }
                else if (fileName == "f.csv")
                {
                    out << arrf[k][m] << ",";
                }
                else if (fileName == "g.csv")
                {
                    out << arrg[k][m] << ",";
                }
                else
                {
                    out << arrh[k][m] << ",";
                }
            }
            out << endl;
        }

        ini.close();

        ini.rename(fileName, folder + "/" + fileName);
    }
}

void ForThread3::saveFileInRegisterRegression(QString fileName, QString folder)
{

    //ファイルオープン
    QFile file(folder + "/" + fileName);

    //ファイルが存在する場合
    if (file.exists())
    {
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);

        //データを出力
        for (int k = 0; k < 248; k++)
        {
            for (int m = 0; m < 328; m++)
            {
                if (fileName == "residual.csv")
                {
                    out << residual[k][m] << ",";
                }
                else if (fileName == "a.csv")
                {
                    out << arra[k][m] << ",";
                }
                else if (fileName == "b.csv")
                {
                    out << arrb[k][m] << ",";
                }
                else if (fileName == "c.csv")
                {
                    out << arrc[k][m] << ",";
                }
                else if (fileName == "d.csv")
                {
                    out << arrd[k][m] << ",";
                }
                else if (fileName == "e.csv")
                {
                    out << arre[k][m] << ",";
                }
                else if (fileName == "f.csv")
                {
                    out << arrf[k][m] << ",";
                }
                else if (fileName == "g.csv")
                {
                    out << arrg[k][m] << ",";
                }
                else
                {
                    out << arrh[k][m] << ",";
                }
            }
            out << endl;
        }

        file.close();
    }
    else
    { //ファイルが存在しなかった場合
        //ファイル作成
        QFile ini(fileName);

        ini.open(QIODevice::WriteOnly);

        QTextStream out(&ini);

        for (int k = 0; k < 248; k++)
        {
            for (int m = 0; m < 328; m++)
            {
                if (fileName == "residual.csv")
                {
                    out << residual[k][m] << ",";
                }
                else if (fileName == "a.csv")
                {
                    out << arra[k][m] << ",";
                }
                else if (fileName == "b.csv")
                {
                    out << arrb[k][m] << ",";
                }
                else if (fileName == "c.csv")
                {
                    out << arrc[k][m] << ",";
                }
                else if (fileName == "d.csv")
                {
                    out << arrd[k][m] << ",";
                }
                else if (fileName == "e.csv")
                {
                    out << arre[k][m] << ",";
                }
                else if (fileName == "f.csv")
                {
                    out << arrf[k][m] << ",";
                }
                else if (fileName == "g.csv")
                {
                    out << arrg[k][m] << ",";
                }
                else
                {
                    out << arrh[k][m] << ",";
                }
            }
            out << endl;
        }

        ini.close();

        ini.rename(fileName, folder + "/" + fileName);
    }
}

void ForThread4::saveFileInRegisterRegression(QString fileName, QString folder)
{

    //ファイルオープン
    QFile file(folder + "/" + fileName);

    //ファイルが存在する場合
    if (file.exists())
    {
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);

        //データを出力
        for (int k = 0; k < 248; k++)
        {
            for (int m = 0; m < 328; m++)
            {
                if (fileName == "residual.csv")
                {
                    out << residual[k][m] << ",";
                }
                else if (fileName == "a.csv")
                {
                    out << arra[k][m] << ",";
                }
                else if (fileName == "b.csv")
                {
                    out << arrb[k][m] << ",";
                }
                else if (fileName == "c.csv")
                {
                    out << arrc[k][m] << ",";
                }
                else if (fileName == "d.csv")
                {
                    out << arrd[k][m] << ",";
                }
                else if (fileName == "e.csv")
                {
                    out << arre[k][m] << ",";
                }
                else if (fileName == "f.csv")
                {
                    out << arrf[k][m] << ",";
                }
                else if (fileName == "g.csv")
                {
                    out << arrg[k][m] << ",";
                }
                else
                {
                    out << arrh[k][m] << ",";
                }
            }
            out << endl;
        }

        file.close();
    }
    else
    { //ファイルが存在しなかった場合
        //ファイル作成
        QFile ini(fileName);

        ini.open(QIODevice::WriteOnly);

        QTextStream out(&ini);

        for (int k = 0; k < 248; k++)
        {
            for (int m = 0; m < 328; m++)
            {
                if (fileName == "residual.csv")
                {
                    out << residual[k][m] << ",";
                }
                else if (fileName == "a.csv")
                {
                    out << arra[k][m] << ",";
                }
                else if (fileName == "b.csv")
                {
                    out << arrb[k][m] << ",";
                }
                else if (fileName == "c.csv")
                {
                    out << arrc[k][m] << ",";
                }
                else if (fileName == "d.csv")
                {
                    out << arrd[k][m] << ",";
                }
                else if (fileName == "e.csv")
                {
                    out << arre[k][m] << ",";
                }
                else if (fileName == "f.csv")
                {
                    out << arrf[k][m] << ",";
                }
                else if (fileName == "g.csv")
                {
                    out << arrg[k][m] << ",";
                }
                else
                {
                    out << arrh[k][m] << ",";
                }
            }
            out << endl;
        }

        ini.close();

        ini.rename(fileName, folder + "/" + fileName);
    }
}

void ForThread5::saveFileInRegisterRegression(QString fileName, QString folder)
{

    //ファイルオープン
    QFile file(folder + "/" + fileName);

    //ファイルが存在する場合
    if (file.exists())
    {
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);

        //データを出力
        for (int k = 0; k < 248; k++)
        {
            for (int m = 0; m < 328; m++)
            {
                if (fileName == "residual.csv")
                {
                    out << residual[k][m] << ",";
                }
                else if (fileName == "a.csv")
                {
                    out << arra[k][m] << ",";
                }
                else if (fileName == "b.csv")
                {
                    out << arrb[k][m] << ",";
                }
                else if (fileName == "c.csv")
                {
                    out << arrc[k][m] << ",";
                }
                else if (fileName == "d.csv")
                {
                    out << arrd[k][m] << ",";
                }
                else if (fileName == "e.csv")
                {
                    out << arre[k][m] << ",";
                }
                else if (fileName == "f.csv")
                {
                    out << arrf[k][m] << ",";
                }
                else if (fileName == "g.csv")
                {
                    out << arrg[k][m] << ",";
                }
                else
                {
                    out << arrh[k][m] << ",";
                }
            }
            out << endl;
        }

        file.close();
    }
    else
    { //ファイルが存在しなかった場合
        //ファイル作成
        QFile ini(fileName);

        ini.open(QIODevice::WriteOnly);

        QTextStream out(&ini);

        for (int k = 0; k < 248; k++)
        {
            for (int m = 0; m < 328; m++)
            {
                if (fileName == "residual.csv")
                {
                    out << residual[k][m] << ",";
                }
                else if (fileName == "a.csv")
                {
                    out << arra[k][m] << ",";
                }
                else if (fileName == "b.csv")
                {
                    out << arrb[k][m] << ",";
                }
                else if (fileName == "c.csv")
                {
                    out << arrc[k][m] << ",";
                }
                else if (fileName == "d.csv")
                {
                    out << arrd[k][m] << ",";
                }
                else if (fileName == "e.csv")
                {
                    out << arre[k][m] << ",";
                }
                else if (fileName == "f.csv")
                {
                    out << arrf[k][m] << ",";
                }
                else if (fileName == "g.csv")
                {
                    out << arrg[k][m] << ",";
                }
                else
                {
                    out << arrh[k][m] << ",";
                }
            }
            out << endl;
        }

        ini.close();

        ini.rename(fileName, folder + "/" + fileName);
    }
}

void ForThread6::saveFileInRegisterRegression(QString fileName, QString folder)
{

    //ファイルオープン
    QFile file(folder + "/" + fileName);

    //ファイルが存在する場合
    if (file.exists())
    {
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);

        //データを出力
        for (int k = 0; k < 248; k++)
        {
            for (int m = 0; m < 328; m++)
            {
                if (fileName == "residual.csv")
                {
                    out << residual[k][m] << ",";
                }
                else if (fileName == "a.csv")
                {
                    out << arra[k][m] << ",";
                }
                else if (fileName == "b.csv")
                {
                    out << arrb[k][m] << ",";
                }
                else if (fileName == "c.csv")
                {
                    out << arrc[k][m] << ",";
                }
                else if (fileName == "d.csv")
                {
                    out << arrd[k][m] << ",";
                }
                else if (fileName == "e.csv")
                {
                    out << arre[k][m] << ",";
                }
                else if (fileName == "f.csv")
                {
                    out << arrf[k][m] << ",";
                }
                else if (fileName == "g.csv")
                {
                    out << arrg[k][m] << ",";
                }
                else
                {
                    out << arrh[k][m] << ",";
                }
            }
            out << endl;
        }

        file.close();
    }
    else
    { //ファイルが存在しなかった場合
        //ファイル作成
        QFile ini(fileName);

        ini.open(QIODevice::WriteOnly);

        QTextStream out(&ini);

        for (int k = 0; k < 248; k++)
        {
            for (int m = 0; m < 328; m++)
            {
                if (fileName == "residual.csv")
                {
                    out << residual[k][m] << ",";
                }
                else if (fileName == "a.csv")
                {
                    out << arra[k][m] << ",";
                }
                else if (fileName == "b.csv")
                {
                    out << arrb[k][m] << ",";
                }
                else if (fileName == "c.csv")
                {
                    out << arrc[k][m] << ",";
                }
                else if (fileName == "d.csv")
                {
                    out << arrd[k][m] << ",";
                }
                else if (fileName == "e.csv")
                {
                    out << arre[k][m] << ",";
                }
                else if (fileName == "f.csv")
                {
                    out << arrf[k][m] << ",";
                }
                else if (fileName == "g.csv")
                {
                    out << arrg[k][m] << ",";
                }
                else
                {
                    out << arrh[k][m] << ",";
                }
            }
            out << endl;
        }

        ini.close();

        ini.rename(fileName, folder + "/" + fileName);
    }
}



void CalibrationGraph::on_loadFileButton_clicked()
{
    //Dialogでファイル名(パス)取得
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open Image"), QDir::homePath(), tr("Image Files (*.fit *.fits *.fts *.inf)"));
    ImageFilefilepath = fileName;
    //  std::cout<<fileName.toStdString()<<std::endl;
    ui->fileNameBrowser->clear();
    ui->fileNameBrowser->setText(fileName.section('/', -1, -1));

    fstream ifs;
    ifs.open(&fileName.toStdString()[0], ios::in | ios::binary);
    QFileInfo fileinfo;
    fileinfo.setFile(fileName);
    QString ext = fileinfo.suffix();
    ext = ext.toLower();

    if (ext == "fit" || ext == "fits" || ext == "fts") //(fitファイルの検索)
    {
        valarray<long> contents;
        auto_ptr<FITS> pInfile(0);
        try
        {
            pInfile.reset(new FITS(fileName.toStdString().c_str(), Read, true));
            PHDU &fitsImage = pInfile->pHDU();
            fitsImage.read(contents);
            fitsImage.readAllKeys();

            pInfile->pHDU().readKey<double>("BOL_TEMP", bol_temp);
            pInfile->pHDU().readKey<double>("PKG_TEMP", pkg_temp);
            pInfile->pHDU().readKey<double>("CAS_TEMP", cas_temp);
            pInfile->pHDU().readKey<double>("SHT_TEMP", sht_temp);
            pInfile->pHDU().readKey<double>("LEN_TEMP", len_temp);
            fitsfilename = "File Name: " + QFileInfo(fileName).fileName(); //ファイル名

            ui->pkgMaxSlider->setValue(pkg_temp * 1000 + 2000);
            ui->pkgMinSlider->setValue(pkg_temp * 1000 - 1000);

            ui->caseMaxSlider->setValue(cas_temp * 1000 + 2000);
            ui->caseMinSlider->setValue(cas_temp * 1000 - 1000);

            ui->shMaxSlider->setValue(sht_temp * 1000 + 2000);
            ui->shMinSlider->setValue(sht_temp * 1000 - 1000);

            ui->lensMaxSlider->setValue(len_temp * 1000 + 2000);
            ui->lensMinSlider->setValue(len_temp * 1000 - 1000);

            ui->pkg_tempBrowser->clear();
            ui->pkg_tempBrowser->setText(QString::number(pkg_temp));

            ui->cas_tempBrowser->clear();
            ui->cas_tempBrowser->setText(QString::number(cas_temp));
            ui->sht_tempBrowser->clear();
            ui->sht_tempBrowser->setText(QString::number(sht_temp));
            ui->len_tempBrowser->clear();
            ui->len_tempBrowser->setText(QString::number(len_temp));

            /*
                                            cout<<bol_temp<<endl;
                                            cout<<pkg_temp<<endl;
                                            cout<<cas_temp<<endl;
                                            cout<<sht_temp<<endl;
                                            cout<<len_temp<<endl;
                                */
        }
        catch (...)
        {
        };
    }

    if (ext == "inf")
    {
        double imagetmp[50][2];
        QFile file1(fileinfo.filePath());
        if (!file1.open(QIODevice::ReadOnly)) //読込のみでオープンできたかチェック
        {
            printf("txt dat open error\n");
            return;
        }
        QTextStream in1(&file1);
        QString str1;
        //係数読み込み
        for (int i = 0; !in1.atEnd(); i++)
        {
            for (int j = 0; j < 2; j++)
            {
                in1 >> str1;
                //要素x,y,z
                imagetmp[i][j] = str1.toDouble();
            }
        }

        bol_temp = imagetmp[38][1];
        pkg_temp = imagetmp[42][1];
        cas_temp = imagetmp[43][1];
        sht_temp = imagetmp[44][1];
        len_temp = imagetmp[45][1];

        ui->pkgMaxSlider->setValue(pkg_temp * 1000 + 1000);
        ui->pkgMinSlider->setValue(pkg_temp * 1000 - 1000);

        ui->caseMaxSlider->setValue(cas_temp * 1000 + 1000);
        ui->caseMinSlider->setValue(cas_temp * 1000 - 1000);

        ui->shMaxSlider->setValue(sht_temp * 1000 + 1000);
        ui->shMinSlider->setValue(sht_temp * 1000 - 1000);

        ui->lensMaxSlider->setValue(len_temp * 1000 + 1000);
        ui->lensMinSlider->setValue(len_temp * 1000 - 1000);

        ui->pkg_tempBrowser->clear();
        ui->pkg_tempBrowser->setText(QString::number(pkg_temp));

        ui->cas_tempBrowser->clear();
        ui->cas_tempBrowser->setText(QString::number(cas_temp));
        ui->sht_tempBrowser->clear();
        ui->sht_tempBrowser->setText(QString::number(sht_temp));
        ui->len_tempBrowser->clear();
        ui->len_tempBrowser->setText(QString::number(len_temp));
    }

    double tgt_max, tgt_min, bolo_max, bolo_min, pkg_max, pkg_min;
    double case_max, case_min, sh_max, sh_min, lens_max, lens_min;
    int count = 0;

    //vx、vyを初期化
    vx.clear();
    vy.clear();

    //入力されている範囲を取得
    tgt_max = ui->tgtMaxLineEdit->text().toDouble();
    tgt_min = ui->tgtMinLineEdit->text().toDouble();

    bolo_max = ui->boloMaxLineEdit->text().toDouble();
    bolo_min = ui->boloMinLineEdit->text().toDouble();

    pkg_max = ui->pkgMaxLineEdit->text().toDouble();
    pkg_min = ui->pkgMinLineEdit->text().toDouble();

    case_max = ui->caseMaxLineEdit->text().toDouble();
    case_min = ui->caseMinLineEdit->text().toDouble();

    sh_max = ui->shMaxLineEdit->text().toDouble();
    sh_min = ui->shMinLineEdit->text().toDouble();

    lens_max = ui->lensMaxLineEdit->text().toDouble();
    lens_min = ui->lensMinLineEdit->text().toDouble();

    QVector<QString> tmp;
    replot.clear();

    //diffの時はサーチペアを何回もしなようにdiff専用の処理をする
    int f = judgeAxis(xAxis, yAxis);

    if ((f == 1 || f == 2) && (xAxis == "diff DN" || yAxis == "diff DN"))
    {
        //tmpに条件が合致したピクセルの情報を格納
        count = 0;
        for (int i = 0; i < infoNum; i++)
        {
            if (tgt_min <= diff[i][9].toDouble() && diff[i][9].toDouble() <= tgt_max)
            {
                if (bolo_min <= diff[i][4].toDouble() && diff[i][4].toDouble() <= bolo_max)
                {
                    if (pkg_min <= diff[i][5].toDouble() && diff[i][5].toDouble() <= pkg_max)
                    {
                        if (case_min <= diff[i][6].toDouble() && diff[i][6].toDouble() <= case_max)
                        {
                            if (sh_min <= diff[i][7].toDouble() && diff[i][7].toDouble() <= sh_max)
                            {
                                if (lens_min <= diff[i][8].toDouble() && diff[i][8].toDouble() <= lens_max)
                                {
                                    tmp.clear();
                                    for (int j = 0; j < 25; j++)
                                    {
                                        tmp.append(diff[i][j]);
                                    }
                                    replot.append(tmp);
                                    count++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        //tmpに条件が合致したピクセルの情報を格納
        count = 0;
        for (int i = 0; i < previousNum; i++)
        {
            if (tgt_min <= pixelList[i][9].toDouble() && pixelList[i][9].toDouble() <= tgt_max)
            {
                if (bolo_min <= pixelList[i][4].toDouble() && pixelList[i][4].toDouble() <= bolo_max)
                {
                    if (pkg_min <= pixelList[i][5].toDouble() && pixelList[i][5].toDouble() <= pkg_max)
                    {
                        if (case_min <= pixelList[i][6].toDouble() && pixelList[i][6].toDouble() <= case_max)
                        {
                            if (sh_min <= pixelList[i][7].toDouble() && pixelList[i][7].toDouble() <= sh_max)
                            {
                                if (lens_min <= pixelList[i][8].toDouble() && pixelList[i][8].toDouble() <= lens_max)
                                {
                                    tmp.clear();
                                    for (int j = 0; j < 25; j++)
                                    {
                                        tmp.append(pixelList[i][j]);
                                    }
                                    replot.append(tmp);
                                    count++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //軸の名前を元に各種処理をする
    if (f == -1)
    {
        vx = getAxisValue(xAxis, replot, count);
        vy = getAxisValue(yAxis, replot, count);
    }
    else if (f == 0)
    {
        vx = getAxisValue2(xAxis, replot, count);
        vy = getAxisValue2(yAxis, replot, count);
    }
    else if (f == 1)
    {
        if (xAxis == "diff DN")
        {
            vx = getAxisValue(xAxis, replot, count);
            vy = getAxisValue(yAxis, replot, count);
        }
        else
        {
            vx = getAxisValue2(xAxis, replot, count);
            vy = getAxisValue(yAxis, pixelList2, infoNum);
        }
    }
    else if (f == 2)
    {
        if (yAxis == "diff DN")
        {
            vy = getAxisValue(yAxis, replot, count);
            vx = getAxisValue(xAxis, replot, count);
        }
        else
        {
            vy = getAxisValue2(yAxis, replot, count);
            vx = getAxisValue(xAxis, pixelList2, infoNum);
        }
    }

    //プロットしている数のセット
    ui->plotNumberBrowser->clear();
    if (f == -1)
    {
        ui->plotNumberBrowser->setText(QString::number(count) + " / " + QString::number(previousNum));
    }
    else if (xAxis == "diff DN" || yAxis == "diff DN")
    {
        ui->plotNumberBrowser->setText(QString::number(count) + " / " + QString::number(plotNum));
    }
    else
    {
        ui->plotNumberBrowser->setText(QString::number(infoNum) + " / " + QString::number(plotNum));
    }
    //cout<<"plotnum(絞り込み後)";
    //cout<<count<<endl;
    Outputplotnumber = count;

    //グラフの描画
    drawGraph(vx, vy, "");
}

//積分計算関数
double CalibrationGraph::planck(double T)
{
    //double x0=8e-6, x1=12e-6; //8μm~12μm 領域
    //double x0=1e-8, x1=20e-6; //
    double lambda, Bt, integral = 0, epsilon = 0.925;
    // 1000=10μm　100000=100μm

    for (int i = 1; i < 2001; i++)
    {
        lambda = (double)i * 1e-8; // 0.00000001m=0.01μm step
        Bt = ((2 * h_planck * c_speed * c_speed) / (pow(lambda, 5)) / (pow(M_E, c2 / (lambda * T)) - 1.0) * tirfilter[i][1] * epsilon);
        integral += (Bt); //資料のF(T)の予想されるフラックスのこと
    }
    integral *= 1e-8; //0.00000001
    return integral;
}

double planck4(double T)
{
    //double x0=8e-6, x1=12e-6; //8μm~12μm 領域
    //double x0=1e-8, x1=20e-6; //
    double lambda, Bt, integral = 0, epsilon = 0.925;
    double h_planck = 6.62606957e-34;
    double kB = 1.3806488e-23;
    double c_speed = 299792458;
    double c1 = 2 * M_PI * h_planck * pow(c_speed, 2);
    double c2 = (h_planck * c_speed) / kB;
    double SIGMA = 5.670373e-8;

    // 1000=10μm　100000=100μm

    for (int i = 1; i < 2001; i++)
    {
        lambda = (double)i * 1e-8; // 0.00000001m=0.01μm step
        Bt = ((2 * h_planck * c_speed * c_speed) / (pow(lambda, 5)) / (pow(M_E, c2 / (lambda * T)) - 1.0) * tirfilter[i][1] * epsilon);
        integral += (Bt); //資料のF(T)の予想されるフラックスのこと
    }
    integral *= 1e-8; //0.00000001
    return integral;
}

void CalibrationGraph::loadFilter()
{

    QString str;
    //バンドパスフィルタ読み込み
    QString appPath;
    //初期設定ファイル格納用ディレクトリ
    appPath = QCoreApplication::applicationDirPath();

    qDebug()<<appPath;
    QFile file(appPath + "/tir_response.txt"); //TIRの応答関数R(λ)のデータ

    if (!file.open(QIODevice::ReadOnly)) //読込のみでオープンできたかチェック
    {
        printf("tir_response.txt open error\n");
        return;
    }

    QTextStream in(&file);

    //係数読み込み
    for (int i = 0; !in.atEnd(); i++)
    {
        for (int j = 0; j < 3; j++)
        {
            in >> str;
            tirfilter[i][j] = str.toDouble();
        }
    }
}
