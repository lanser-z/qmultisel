#ifndef RANGETABLE_H
#define RANGETABLE_H

#include <QTableView>
#include <QTime>
#include "rangetypes.h"

class RangeTable : public QTableView
{
public:
    explicit RangeTable(QWidget* parent, int rowHeadWidth=100);
    virtual ~RangeTable();

    void SetHeader(const QStringList& headerTexts, int columnWidth, Qt::Alignment alignment=Qt::AlignLeft);
    void SetRows(const QStringList& rowHeadTexts, int rowHeight);
    void SetupLayout(int timeSpanSeconds);
    void SetSelectionMode(bool selectToAdd);
    void ResetSelection();

    void AddCellData(int row, int col, const QImage& data);

    QVector<QVector<TimeRange> > GetSelectionTimes() const;
    QVector<RowTimeRange> GetRowTimes() const;

private:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void leaveEvent(QEvent *event);
    void ProcessNewSelection();
    void EndGrab();

private:
    QVector<QVector<PixelRange> > m_selections;
    RowPixelRange m_newSelection;

    int m_rowHeadWidth;
    int m_columnWidth;
    int m_rowHeight;
    int m_timeSpanSeconds;

    QStringList m_headTexts;
    QStringList m_rowTexts;

    bool m_select2Add;
    bool m_grabNow;

    class Cursor : public QWidget
    {
    public:
        explicit Cursor(QWidget* parent);
        virtual ~Cursor();

        void CenterOn(int xPos, int xOffset);
        void SetLabelMap(int xRange, int totalSeconds);

    private:
        virtual void paintEvent(QPaintEvent* event);

    private:
        int m_xRange;
        int m_totalSeconds;
        int m_xOffset;
    };
    Cursor* m_cursorPtr;

};

#endif // RANGETABLE_H
