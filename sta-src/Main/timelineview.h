/*
 This program is free software; you can redistribute it and/or modify it under
 the terms of the European Union Public Licence - EUPL v.1.1 as published by
 the European Commission.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the European Union Public Licence - EUPL v.1.1
 for more details.

 You should have received a copy of the European Union Public Licence - EUPL v.1.1
 along with this program.

 Further information about the European Union Public Licence - EUPL v.1.1 can
 also be found on the world wide web at http://ec.europa.eu/idabc/eupl

*/

/*
 ------ Copyright (C) 2010 STA Steering Board (space.trajectory.analysis AT gmail.com) ----
*/
/*
------------------ Author: Chris Laurel  -------------------------------------------------
 ------------------ E-mail: (claurel@gmail.com) ----------------------------
 */

#ifndef _STA_MAIN_TIMELINEVIEW_H_
#define _STA_MAIN_TIMELINEVIEW_H_

#include <QAbstractScrollArea>
#include <QList>

class QTimer;

class TimelineView : public QAbstractScrollArea
{
Q_OBJECT
 public:
    TimelineView(QWidget *parent = 0);
    ~TimelineView();
    
    void setTimeRange(double startTime, double endTime);
    double startTime() const { return m_startTime; }
    double endTime() const { return m_endTime; }
    
    void setVisibleSpan(double duration);
    double visibleSpan() const { return m_visibleSpan; }
    
    void clearMission();
    void addMissionSegment(int participantIndex,
                           double startTime,
                           double endTime,
                           QColor color,
                           const QString& name);

    double currentTime() const { return m_currentTime; }

 public slots:
    void setCurrentTime(double mjd);
    
 signals:
    void participantClicked(int participantIndex);
    void currentTimeChanged(double currentTime);

 protected:
    void paintEvent(QPaintEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void resizeEvent(QResizeEvent* event);
    
 private:
    void updateScrollBars();

 private slots:
    void animate();
    
 private:
    double m_currentTime;
    double m_startTime;
    double m_endTime;
    double m_visibleSpan;
    int m_participantCount;
    
    struct MissionSegment
    {
        int participantIndex;
        double startTime;
        double endTime;
        QColor color;
        QString name;
    };
    
    QList<MissionSegment> mission;

    float m_mouseMotion;
    QPointF m_lastMousePosition;

    QTimer* m_timer;
    double m_autoScrollRate;
    double m_targetVisibleSpan;
    double m_zoomScaleFactor;
};

#endif // _STA_MAIN_TIMELINEVIEW_H_





