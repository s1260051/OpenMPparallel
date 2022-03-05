#ifndef PIXCELGRAPH_H
#define PIXCELGRAPH_H

#include <QDialog>


#define Width 384
#define Height 256

namespace Ui {
class PixcelGraph;
}

class PixcelGraph : public QDialog
{
    Q_OBJECT

public:
    explicit PixcelGraph(QWidget *parent = 0);
    ~PixcelGraph();

    int flag = 0;
    void drawGraph(QVector<double>, QVector<double>);

private:
    Ui::PixcelGraph *ui;

};

#endif // PIXCELGRAPH_H
