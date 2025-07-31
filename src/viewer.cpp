#include "viewer.h"

Viewer::Viewer(QWidget *parent)
  : QWidget(parent), viewer(new QPlainTextEdit(this)) {
  setWindowTitle("Deskflow Control");
  resize(800, 600);
  viewer->setReadOnly(true);
  viewer->setMaximumBlockCount(500);
  setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
  QPushButton *clearButton = new QPushButton("Clear Log");
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addWidget(viewer);
  layout->addWidget(clearButton);
  setLayout(layout);
  connect(clearButton, &QPushButton::clicked, viewer, &QPlainTextEdit::clear);
}

void Viewer::appendLog(const QString &text, const QColor &color) {
  QTextCharFormat format;
  format.setForeground(color);
  QTextCursor cursor = viewer->textCursor();
  cursor.movePosition(QTextCursor::End);
  cursor.insertText(text + "\n", format);
  viewer->setTextCursor(cursor);
  viewer->ensureCursorVisible();
}

void Viewer::closeEvent(QCloseEvent *event) {
  event->ignore();
  this->hide();
  emit visibilityChanged(false);
}
