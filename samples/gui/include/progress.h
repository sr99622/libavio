/********************************************************************
* libavio/samples/gui/include/progress.h
*
* Copyright (c) 2023  Stephen Rhodes
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*********************************************************************/

#ifndef PROGRESS_H
#define PROGRESS_H

#include <QSlider>
#include <QLabel>

class ProgressSlider : public QSlider
{
    Q_OBJECT

public:
    ProgressSlider(Qt::Orientation orientation, QWidget* parent = nullptr);

protected:
    bool event(QEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

};

class PositionLabel : public QLabel
{
    Q_OBJECT

public:
    PositionLabel(QWidget* parent);
    void setText(const QString& arg, int pos);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_pos = 0;

};

class Progress : public QWidget
{
    Q_OBJECT

public:
    Progress(QWidget* parent = nullptr);

    void setDuration(qint64 arg);
    qint64 duration() { return m_duration; }
    void setProgress(float pct);
    QString getTimeString(qint64 milliseconds) const;

    ProgressSlider* sldProgress;
    QLabel* lblProgress;
    QLabel* lblDuration;
    PositionLabel* lblPosition;

signals:
    void seek(float);

private:
    void setLabelWidth(QLabel* lbl);
    void setLabelHeight(QLabel* lbl);
    qint64 m_duration = 0;

};


#endif // PROGRESS_H