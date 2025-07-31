#ifndef VIEWER_H
#define VIEWER_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDateTime>
#include <QCloseEvent>

class Viewer : public QWidget {
    Q_OBJECT

public:
    explicit Viewer(QWidget *parent = nullptr);
    void appendLog(const QString &text, const QColor &color = Qt::black);
    
protected:
    void closeEvent(QCloseEvent *event) override;

signals:
    void visibilityChanged(bool visible);

private:
    QPlainTextEdit *viewer;
};

#endif // VIEWER_H
