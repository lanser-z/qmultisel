#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QRadioButton>
#include "rangetable.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void SetupLayout();

private:
    RangeTable m_rangeTable;
    QRadioButton m_addButton;
    QRadioButton m_subButton;
};

#endif // MAINWINDOW_H
