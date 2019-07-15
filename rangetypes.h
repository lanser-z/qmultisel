#ifndef RANGETYPES_H
#define RANGETYPES_H

#include <QTime>
#include <QVector>

typedef struct _tagTimeRange
{
    QTime begin;
    QTime end;

    _tagTimeRange()
    {
        begin.setHMS(0, 0, 0);
        end.setHMS(0, 0, 0);
    }
    _tagTimeRange(const _tagTimeRange& other)
    {
        begin = other.begin;
        end = other.end;
    }
    const _tagTimeRange& operator = (const _tagTimeRange& other)
    {
        begin = other.begin;
        end = other.end;
        return *this;
    }
} TimeRange, *PTimeRange;

typedef struct _tagRowTimeRange : public TimeRange
{
    int row;

    _tagRowTimeRange()
    {
        row = -1;
    }
    _tagRowTimeRange(const _tagRowTimeRange& other)
    {
        row = other.row;
        begin = other.begin;
        end = other.end;
    }
    const _tagRowTimeRange& operator = (const _tagRowTimeRange& other)
    {
        row = other.row;
        begin = other.begin;
        end = other.end;
        return *this;
    }
} RowTimeRange, *PRowTimeRange;

typedef struct _tagPixelRange {
    int start;
    int end;

    _tagPixelRange() {
        start = end = -1;
    }
    _tagPixelRange(const _tagPixelRange& other)
    {
        start = other.start;
        end = other.end;
    }
    const _tagPixelRange& operator = (const _tagPixelRange& other)
    {
        start = other.start;
        end = other.end;
        return *this;
    }
    bool operator == (const _tagPixelRange & other) const
    {
        return start == other.start && end == other.end;
    }

    bool IsValid() const {
        return start != -1 && end != -1;
    }
    bool IsContain(int pos) const
    {
        return pos >= start && pos <= end;
    }
    void Normalize()
    {
        if (start > end)
        {
            std::swap(start, end);
        }
    }

    _tagPixelRange Intersection(const _tagPixelRange& other)
    {
        _tagPixelRange intersection;
        intersection.start = start > other.start ? start : other.start;
        intersection.end = end < other.end ? end : other.end;
        if (intersection.start > intersection.end)
        {
            intersection.start = intersection.end = -1;
        }
        return intersection;
    }
    void Supplementary(const _tagPixelRange& other, _tagPixelRange& left, _tagPixelRange& right)
    {
        _tagPixelRange intersection = Intersection(other);

        // 如果没有交集，补集为自身
        if (!intersection.IsValid())
        {
            left = right = *this;
        }
        // 如果交集等于自身，补集为空
        else if (intersection == *this)
        {
            left = right = _tagPixelRange();
        }
        // 如果交集左对齐，补集只有右边
        else if (intersection.start == start)
        {
            left = _tagPixelRange();
            right.start = intersection.end + 1;
            right.end = end;
        }
        // 如果交集右对齐，补集只有左边
        else if (intersection.end == end)
        {
            left.start = start;
            left.end = intersection.start - 1;
            right = _tagPixelRange();
        }
        // 交集在中间，左右各有一段补集
        else
        {
            left.start = start;
            left.end = intersection.start - 1;
            right.start = intersection.end + 1;
            right.end = end;
        }
    }

    static QVector<_tagPixelRange> DiscardInvalidSelections(const QVector<_tagPixelRange>& selections)
    {
        QVector<_tagPixelRange> valids;
        valids.reserve(selections.size());
        for (int i = 0; i < selections.size(); ++i)
        {
            if (selections[i].IsValid())
            {
                valids.push_back(selections[i]);
            }
        }
        return valids;
    }
} PixelRange, *PPixelRange;

typedef struct _tagRowPixelRange : public PixelRange
{
    int row;
    _tagRowPixelRange() {
        row = -1;
    }
    void Reset() {
        start = end = row = -1;
    }
    bool IsValid() const {
        return PixelRange::IsValid() && row != -1;
    }
    PixelRange GetRange() {
        PixelRange range;
        range.start = start;
        range.end = end;
        return range;
    }
    void UpdateRange(const PixelRange& newRange)
    {
        start = newRange.start;
        end = newRange.end;
    }
} RowPixelRange, *PRowPixelRange;

#endif // RANGETYPES_H
