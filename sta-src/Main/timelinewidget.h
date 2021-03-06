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

#ifndef _MAIN_TIMELINEWIDGET_H_
#define _MAIN_TIMELINEWIDGET_H_

#include <QWidget>

class TimelineView;

class TimelineWidget : public QWidget
{
Q_OBJECT
 public:
    TimelineWidget(QWidget* parent = 0);
    ~TimelineWidget();
    
    TimelineView* timelineView() const { return m_timelineView; }
    
    double timeRate() const
    {
        return m_timeRate;
    }

    bool isPaused() const
    {
        return m_paused;
    }

    double currentTime() const;

 public slots:
    void setCurrentTime(double mjd);
    void setTimeRate(double timeRate);
    void setPaused(bool pause);
    void tick(double dt);

    void setZoom(const QString& duration);
    
    void reverseTime();
    void slowerTime();
    void halfTime();
    void togglePaused();
    void realTime();
    void doubleTime();
    void fasterTime();

 signals:
    void currentTimeChanged(double currentTime);

 private:
    TimelineView* m_timelineView;
    double m_timeRate;
    bool m_paused;
};

#endif // _MAIN_TIMELINEWIDGET_H_
