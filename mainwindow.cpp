#include "mainwindow.h"

#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), m_rangeTable(this), m_addButton("+", this), m_subButton("-", this)
{
    setMinimumSize(800, 600);
    setWindowTitle("Merge multiple videos");
    m_addButton.move(0, -2);
    m_subButton.move(50, -2);
    m_addButton.setChecked(true);

    connect(&m_addButton, &QRadioButton::pressed, [this]{m_rangeTable.SetSelectionMode(true);} );
    connect(&m_subButton, &QRadioButton::pressed, [this]{m_rangeTable.SetSelectionMode(false);} );
}

MainWindow::~MainWindow()
{

}

void MainWindow::SetupLayout()
{
    // 设置时间轴
    QStringList timeline;
    timeline << "00:00" << "01:00" << "02:00" << "03:00" << "04:00" << "05:00" << "06:00" << "07:00" << "08:00" << "09:00" << "10:00";
    timeline << "10:00" << "11:00" << "12:00" << "13:00" << "14:00" << "15:00" << "16:00" << "17:00" << "18:00" << "19:00" << "20:00";
    m_rangeTable.SetHeader(timeline, 100); // 每列宽100

    // 设置视频名
    QStringList videos;
    videos << "camera 1" << "camera 2" << "camera 3" << "camera 4" << "camera 5"  << "camera 6" << "camera 7" << "camera 8" << "camera 9" << "camera 10";
    m_rangeTable.SetRows(videos, 100); // 每行高100

    // 创建布局并映射到时间范围
    m_rangeTable.SetupLayout(timeline.size()*60); // 每列1min

    setCentralWidget(&m_rangeTable);
}
