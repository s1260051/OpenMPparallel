#include "pixcelgraph.h"
#include "ui_pixcelgraph.h"

PixcelGraph::PixcelGraph(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PixcelGraph)
{
    ui->setupUi(this);
    this->setWindowTitle("Pixcel Line Graph");
}

PixcelGraph::~PixcelGraph()
{
    delete ui;
}

//グラフ描画関数
void PixcelGraph::drawGraph(QVector<double> H, QVector<double> V){

    //初期化
    ui->widget->clearGraphs();

    QCPGraph *graph = new QCPGraph(ui->widget->xAxis, ui->widget->yAxis);
    ui->widget->addPlottable(graph);
    ui->widget->xAxis->setLabel("x-coordinate of image"); //横軸
    ui->widget->yAxis->setLabel("digital number"); //横軸
    graph->setData(H, V); //グラフデータをセット
    ui->widget->rescaleAxes(); // 軸の描画
    ui->widget->replot(); //グラフ描画
}
