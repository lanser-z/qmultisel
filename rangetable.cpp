#include "rangetable.h"

#include <QHeaderView>
#include <QPaintEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <stdlib.h>
#include <QDebug>

class ColumnHeader : public QHeaderView
{
public:
    explicit ColumnHeader(QWidget* parent, const QStringList& headerTexts, int columnWidth, int headerHeight, Qt::Alignment alignment)
        : QHeaderView(Qt::Horizontal, parent)
        , m_texts(headerTexts)
        , m_width(columnWidth)
        , m_height(headerHeight)
        , m_align(alignment)
    {
        setSectionResizeMode(QHeaderView::Fixed);
    }
    virtual ~ColumnHeader() {}

private:
    virtual void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
    {
        if (logicalIndex >= 0 && logicalIndex < m_texts.size())
        {
            painter->drawText(rect, m_align|Qt::AlignTop, m_texts[logicalIndex]);
            painter->drawLine(logicalIndex*m_width, height()/2, logicalIndex*m_width, height());

            int section = 6;
            for (int i = 1; i < section; ++i)
            {
                painter->drawLine(logicalIndex*m_width + i*m_width/section, height()-2,
                                  logicalIndex*m_width + i*m_width/section, height());
            }

        }
    }

    virtual QSize sizeHint() const
    {
        return QSize(m_width, m_height);
    }

private:
    QStringList m_texts;
    int m_width;
    int m_height;
    Qt::Alignment m_align;
};

class RowHeader : public QHeaderView
{
public:
    explicit RowHeader(QWidget* parent, const QStringList& rowHeadTexts, int headerWidth, int rowHeight)
        : QHeaderView(Qt::Vertical, parent)
        , m_texts(rowHeadTexts)
        , m_width(headerWidth)
        , m_height(rowHeight)
    {
        setSectionResizeMode(QHeaderView::Fixed);
    }
    virtual ~RowHeader() {}

private:
    virtual void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
    {
        if (logicalIndex >= 0 && logicalIndex < m_texts.size())
        {
            painter->drawText(rect, Qt::AlignLeft|Qt::AlignVCenter, m_texts[logicalIndex]);
        }
    }

    virtual QSize sizeHint() const
    {
        return QSize(m_width, m_height);
    }

private:
    QStringList m_texts;
    int m_width;
    int m_height;
};

Q_DECLARE_METATYPE(QVector<PixelRange>);
Q_DECLARE_METATYPE(RowPixelRange);
class RangeTableModel : public QAbstractTableModel
{
public:
    enum {
        Selections_Role = Qt::UserRole,
        Highlight_Role,
    };

    explicit RangeTableModel(QObject* parent, QVector<QVector<PixelRange> > & selections, RowPixelRange & currentSelection)
        : QAbstractTableModel(parent)
        , m_rowCount(0)
        , m_columnCount(0)
        , m_selections(selections)
        , m_currentSelection(currentSelection)
    {}
    virtual ~RangeTableModel() {}

    void SetDataSize(int rowCount, int columnCount)
    {
        m_rowCount = rowCount;
        m_columnCount = columnCount;

        m_dataMap.clear();
        m_dataMap.reserve(m_rowCount);
        for (int i = 0; i < m_rowCount; ++i)
        {
            m_dataMap.push_back(QVector<QImage>());
            m_dataMap.back().reserve(m_columnCount);
            for (int j = 0; j < m_columnCount; ++j)
            {
                m_dataMap.back().push_back(QImage());
            }
        }
    }

    void AddCellData(int row, int col, const QImage& data)
    {
        if (row >= 0 && row < m_rowCount && col >= 0 && col < m_columnCount)
        {
            m_dataMap[row][col] = data;
        }
    }

    // QAbstractItemModel interface
private:
    int rowCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return m_rowCount;
    }
    int columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return m_columnCount;
    }
    QVariant data(const QModelIndex &index, int role) const
    {
        switch (role)
        {
        case Qt::DisplayRole:
            return m_dataMap[index.row()][index.column()];
        case Selections_Role:
            {
            const QVector<PixelRange>& rowSelections = m_selections[index.row()];
            QVariant roleData;
            roleData.setValue(rowSelections);
            return roleData;
            }
        case Highlight_Role:
            {
            QVariant roleData;
            roleData.setValue(m_currentSelection);
            return roleData;
            }
        default:
            break;
        }
        return QVariant();
    }

private:
    int m_rowCount;
    int m_columnCount;
    QVector<QVector<QImage> > m_dataMap;
    QVector<QVector<PixelRange> >& m_selections;
    RowPixelRange& m_currentSelection;
};

class RangeTableDelegate : public QStyledItemDelegate
{
public:
    explicit RangeTableDelegate(QObject* parent)
        : QStyledItemDelegate(parent) {}
    virtual ~RangeTableDelegate() {}

private:
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QImage image = index.data(Qt::DisplayRole).value<QImage>();
        if (!image.isNull())
        {
            painter->drawImage(option.rect, image);
        }
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

        // 绘制已选范围
        const QVector<PixelRange> & rowSelections = index.data(RangeTableModel::Selections_Role).value<QVector<PixelRange> >();
        PixelRange cellRange;
        cellRange.start = option.rect.left();
        cellRange.end = option.rect.right();
        for (int i = 0; i < rowSelections.size(); ++i)
        {
            PixelRange intersection = cellRange.Intersection(rowSelections[i]);
            if (intersection.IsValid())
            {
                QRect intersectionRect = option.rect;
                intersectionRect.setLeft(intersection.start);
                intersectionRect.setRight(intersection.end);
                painter->fillRect(intersectionRect, QBrush(QColor(0, 0, 255, 128)));
            }
        }

        // 绘制当前选择范围
        RowPixelRange currentSelection = index.data(RangeTableModel::Highlight_Role).value<RowPixelRange>();
        currentSelection.Normalize();
        if (currentSelection.IsValid() && currentSelection.row == index.row())
        {
            PixelRange intersection = cellRange.Intersection(currentSelection);
            if (intersection.IsValid())
            {
                QRect intersectionRect = option.rect;
                intersectionRect.setLeft(intersection.start);
                intersectionRect.setRight(intersection.end);
                painter->fillRect(intersectionRect, QBrush(QColor(0, 255, 0, 64)));
            }
        }
    }
};

RangeTable::RangeTable(QWidget *parent, int rowHeadWidth)
    : QTableView(parent)
    , m_rowHeadWidth(rowHeadWidth)
    , m_timeSpanSeconds(0)
    , m_select2Add(true)
    , m_grabNow(false)
    , m_cursorPtr(nullptr)
{

}

RangeTable::~RangeTable()
{

}

void RangeTable::SetHeader(const QStringList& headerTexts, int columnWidth, Qt::Alignment alignment)
{
    m_headTexts = headerTexts;
    m_columnWidth = columnWidth;

    setHorizontalHeader(new ColumnHeader(this, headerTexts, columnWidth, 20, alignment));
}

void RangeTable::SetRows(const QStringList& rowHeadTexts, int rowHeight)
{
    m_rowTexts = rowHeadTexts;
    m_rowHeight = rowHeight;

    setVerticalHeader(new RowHeader(this, rowHeadTexts, m_rowHeadWidth, rowHeight));
}

void RangeTable::SetupLayout(int timeSpanSeconds)
{
    RangeTableModel* modelPtr = new RangeTableModel(this, m_selections, m_newSelection);
    modelPtr->SetDataSize(m_rowTexts.size(), m_headTexts.size());
    setModel(modelPtr);

    setItemDelegate(new RangeTableDelegate(this));

    setCornerButtonEnabled(false);
    setShowGrid(false);

    setColumnWidth(0, m_rowHeadWidth);
    for (int i = 1; i <= m_headTexts.size(); ++i)
        setColumnWidth(i, m_columnWidth);

    for (int i = 0; i < model()->rowCount(); ++i)
    {
        setRowHeight(i, m_rowHeight);
    }

    delete m_cursorPtr;
    m_cursorPtr = new Cursor(this, m_rowHeadWidth, horizontalHeader()->height());
    m_cursorPtr->setFixedWidth(m_columnWidth);
    m_cursorPtr->CenterOn(0);
    m_cursorPtr->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_cursorPtr->show();

    m_timeSpanSeconds = timeSpanSeconds;

    ResetSelection();
}

void RangeTable::SetSelectionMode(bool selectToAdd)
{
    m_select2Add = selectToAdd;
}

void RangeTable::ResetSelection()
{
    m_selections.clear();
    m_selections.reserve(model()->rowCount());
    for (int i = 0; i < model()->rowCount(); ++i)
    {
        m_selections.push_back(QVector<PixelRange>());
    }
}

QVector<QVector<TimeRange> > RangeTable::GetSelectionTimes() const
{
    QVector<QVector<TimeRange> > timeRangeVector;
    timeRangeVector.reserve(model()->rowCount());
    for (int i = 0; i < m_selections.size(); ++i)
    {
        timeRangeVector.push_back(QVector<TimeRange>());
        QVector<TimeRange> & rowTimeRange = timeRangeVector.back();
        for (int j = 0; j < m_selections[i].size(); ++j)
        {
            rowTimeRange.push_back(TimeRange());
            TimeRange & timeRange = rowTimeRange.back();

            const PixelRange& pixelRange = m_selections[i][j];
            int startSeconds = pixelRange.start * m_timeSpanSeconds / (m_columnWidth*m_headTexts.size());
            int endSeconds = pixelRange.end * m_timeSpanSeconds / (m_columnWidth*m_headTexts.size());
            timeRange.begin = timeRange.begin.addSecs(startSeconds);
            timeRange.end = timeRange.end.addSecs(endSeconds);
        }
    }
    return timeRangeVector;
}

QVector<RowTimeRange> RangeTable::GetRowTimes() const
{
    QVector<QVector<TimeRange> > rangeVector = GetSelectionTimes();
    QList<RowTimeRange> rowRangeList;
    for (int i = 0; i < rangeVector.size(); ++i)
    {
        for (int j = 0; j < rangeVector[i].size(); ++j)
        {
            RowTimeRange timeRange;
            timeRange.row = i;
            timeRange.begin = rangeVector[i][j].begin;
            timeRange.end = rangeVector[i][j].end;
            rowRangeList.push_back(timeRange);
        }
    }
    qSort(rowRangeList.begin(),rowRangeList.end(),
          [](const RowTimeRange &lhs, const RowTimeRange &rhs){return lhs.begin < rhs.begin;});
    return rowRangeList.toVector();
}

void RangeTable::resizeEvent(QResizeEvent *event)
{
    QTableView::resizeEvent(event);
    m_cursorPtr->setFixedHeight(m_rowTexts.size() * rowHeight(0));
    m_cursorPtr->SetLabelMap(m_columnWidth*m_headTexts.size(), m_timeSpanSeconds);
}

void RangeTable::mousePressEvent(QMouseEvent *event)
{
    QTableView::mousePressEvent(event);
    if (event->x() >= 0 && event->x() < m_columnWidth*m_headTexts.size())
    {
        m_cursorPtr->CenterOn(event->x());
        int row = indexAt(event->pos()).row();
        m_newSelection.row = row;
        m_newSelection.start = event->pos().x();
        m_grabNow = true;
    }
}

void RangeTable::mouseMoveEvent(QMouseEvent *event)
{
    QTableView::mouseMoveEvent(event);
    if (m_grabNow)
    {
        if (event->x() >= 0 && event->x() < m_columnWidth*m_headTexts.size())
        {
            m_cursorPtr->CenterOn(event->x());
            m_newSelection.end = event->pos().x();
        }
    }
}

void RangeTable::mouseReleaseEvent(QMouseEvent *event)
{
    QTableView::mouseReleaseEvent(event);
    if (m_grabNow)
    {
        ProcessNewSelection();
        m_newSelection.Reset();
    }
    m_grabNow = false;
}

void RangeTable::leaveEvent(QEvent *event)
{
    QTableView::leaveEvent(event);
    if (m_grabNow)
    {
        ProcessNewSelection();
        m_newSelection.Reset();
    }
    m_grabNow = false;
}

void RangeTable::ProcessNewSelection()
{
    // 不论鼠标拖动方向，先保证start <= end
    m_newSelection.Normalize();
    qDebug() << "select:" << m_newSelection.row << " [" << m_newSelection.start << "," << m_newSelection.end << "]";
    for (int i = 0; i < m_selections.size(); ++i)
    {
        for (int j = 0; j < m_selections[i].size(); ++j)
        {
               qDebug() << "row:" << i << " [" << m_selections[i][j].start << "," << m_selections[i][j].end << "]";
        }
    }

    // 如果是删除选择，只处理选中行即可
    if (!m_select2Add)
    {
        for (int i = 0; i < m_selections[m_newSelection.row].size(); ++i)
        {
            // 把选中部分从当前段中挖掉，得到左右2段补集
            PixelRange left, right;
            m_selections[m_newSelection.row][i].Supplementary(m_newSelection.GetRange(), left, right);

            // 如果什么也不剩下，把当前段移除
            if (!left.IsValid() && !right.IsValid())
            {
                m_selections[m_newSelection.row].remove(i);
                --i;
            }
            // 如果只剩下左边，调整当前段范围
            else if (left.IsValid() && !right.IsValid())
            {
                m_selections[m_newSelection.row][i] = left;
            }
            // 如果只剩下右边，调整当前段范围
            else if (!left.IsValid() && right.IsValid())
            {
                m_selections[m_newSelection.row][i] = right;
            }
            // 如果同时有左右段且范围不一样，按左段调整当前段并插入右段
            else if (!(left == right))
            {
                m_selections[m_newSelection.row][i] = left;
                m_selections[m_newSelection.row].insert(i+1, right);
                break;  // 同时有左右段说明选中范围完全被当前段覆盖，没必要再跟其他段比较
            }
            // 否则当前段不用做任何处理
        }
        dataChanged(model()->index(m_newSelection.row, 0), model()->index(m_newSelection.row, m_headTexts.size()-1));
        return;
    }

    // 如果是增加选择，要先把和其他行重叠部分除去再并入当前行
    QVector<RowPixelRange> validSections;
    validSections.push_back(m_newSelection);
    for (int i = 0; i < m_selections.size(); ++i)
    {
        if (i != m_newSelection.row)
        {
            // 去除和其他行重叠部分
            for (int j = 0; j < m_selections[i].size(); ++j)
            {
                for (int k = 0; k < validSections.size(); ++k)
                {
                    // 把当前段从选中部分中挖掉，得到左右2段补集
                    PixelRange left, right;
                    validSections[k].Supplementary(m_selections[i][j], left, right);

                    // 如果什么也不剩下，把当前段移除
                    if (!left.IsValid() && !right.IsValid())
                    {
                        validSections.remove(k);
                        --k;
                    }
                    // 如果只剩下左边，调整当前段范围
                    else if (left.IsValid() && !right.IsValid())
                    {
                        validSections[k].UpdateRange(left);
                    }
                    // 如果只剩下右边，调整当前段范围
                    else if (!left.IsValid() && right.IsValid())
                    {
                        validSections[k].UpdateRange(right);
                    }
                    // 如果同时有左右段且范围不一样，按左段调整当前段并插入右段
                    else if (!(left == right))
                    {
                        validSections[k].UpdateRange(left);
                        RowPixelRange newAdd;
                        newAdd.row = m_newSelection.row;
                        newAdd.UpdateRange(right);
                        validSections.insert(k+1, newAdd);
                        break;  // 同时有左右段说明当前段完全被选中范围覆盖，没必要再跟其他选中范围对比
                    }
                    // 否则当前段不用做任何处理
                }
            }
        }
    }

    // 把有效（不和其他行已选片段重合）的选中范围并入当前行，添加时保证有序
    QVector<PixelRange>& rowRangeArray = m_selections[m_newSelection.row];
    if (rowRangeArray.empty())
    {
        // 如果当前行没有选过，直接赋值
        for (int i = 0; i < validSections.size(); ++i)
        {
            m_selections[m_newSelection.row].push_back(validSections[i].GetRange());
        }
    }
    else
    {
        // 如果当前行已有选择片段，合并新的，合并中保证有序
        for (int i = 0; i < validSections.size(); ++i)
        {
            // 新选择在最前面
            if (validSections[i].end < rowRangeArray.front().start)
            {
                rowRangeArray.push_front(validSections[i].GetRange());
            }
            // 新选择在最后面
            else if (validSections[i].start > rowRangeArray.back().end)
            {
                rowRangeArray.push_back(validSections[i].GetRange());
            }
            // 新选择在中间
            else
            {
                for (int j = 0; j < rowRangeArray.size(); ++j)
                {
                    // 如果新选择是已有选择的子集，不做任何处理
                    if (rowRangeArray[j].Intersection(validSections[i]) == validSections[i])
                    {
                        break;
                    }
                    // 如果新选择开始于已有选择，检查新选择的结束覆盖了哪些已有选择，做合并
                    else if (rowRangeArray[j].IsContain(validSections[i].start))
                    {
                        int endIndex = -1;
                        for (int k = j+1; k < rowRangeArray.size(); ++k)
                        {
                            if (rowRangeArray[k].IsContain(validSections[i].end))
                            {
                                endIndex = k;
                                break;
                            }
                        }
                        if (endIndex == -1)
                        {
                            // 新选择覆盖所有后续项，全部合并
                            rowRangeArray[j].end = validSections[i].end;
                            rowRangeArray.resize(j+1);
                        }
                        else
                        {
                            // 合并新选择覆盖的项
                            rowRangeArray[j].end = rowRangeArray[endIndex].end;
                            rowRangeArray.remove(j+1, endIndex-j);
                        }
                        break;
                    }
                    // 如果新选择结束于已有选择，检查新选择的结束覆盖了哪些已有选择，做合并
                    else if (rowRangeArray[j].IsContain(validSections[i].end))
                    {
                        int startIndex = -1;
                        for (int k = j-1; k >= 0; --k)
                        {
                            if (rowRangeArray[k].IsContain(validSections[i].start))
                            {
                                startIndex = k;
                                break;
                            }
                        }
                        if (startIndex == -1)
                        {
                            // 新选择覆盖所有前续项，全部合并
                            rowRangeArray[0].start = validSections[i].start;
                            rowRangeArray[0].end = rowRangeArray[j].end;
                            if (j > 0)
                                rowRangeArray.remove(1, j);
                        }
                        else
                        {
                            // 合并新选择覆盖的项
                            rowRangeArray[startIndex].end = rowRangeArray[j].end;
                            rowRangeArray.remove(startIndex+1, j-startIndex);
                        }
                        break;
                    }
                    // 如果新选择处于2个已有选择之间，插入新选择
                    else if (validSections[i].start > rowRangeArray[j].end && validSections[i].end < rowRangeArray[j+1].start)
                    {
                        rowRangeArray.insert(j+1, validSections[i].GetRange());
                        break;
                    }
                }
            }
        }
    }
    dataChanged(model()->index(m_newSelection.row, 0), model()->index(m_newSelection.row, m_headTexts.size()-1));
}

RangeTable::Cursor::Cursor(QWidget *parent, int xOffset, int yOffset)
    : QWidget(parent)
    , m_xOffset(xOffset)
    , m_yOffset(yOffset)
    , m_xRange(0)
    , m_totalSeconds(0)
{

}

RangeTable::Cursor::~Cursor()
{

}

void RangeTable::Cursor::CenterOn(int xPos)
{
    move(m_xOffset + xPos - width()/2, m_yOffset);
}

void RangeTable::Cursor::SetLabelMap(int xRange, int totalSeconds)
{
    m_xRange = xRange;
    m_totalSeconds = totalSeconds;
}

void RangeTable::Cursor::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(QRect(rect().center().x(), rect().top(), 2, rect().height()),
                     QBrush(QColor(255, 0, 0, 100)));
    if (m_xRange > 0 && m_totalSeconds > 0)
    {
        int currentSeconds = (pos().x()+width()/2 - m_xOffset) * m_totalSeconds / m_xRange;
        QString time;
        time.sprintf("%02d:%02d", currentSeconds/60, currentSeconds%60);
        painter.drawText(rect(), Qt::AlignTop|Qt::AlignHCenter, time);
    }
}
