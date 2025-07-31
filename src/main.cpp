#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QProcess>
#include <QIcon>
#include <QDir>
#include <QMessageBox>
#include <QTimer>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QLockFile>
#include "viewer.h"

QProcess *deskflow = nullptr;

bool waitForSystemTray(int timeoutMs = 2000, int intervalMs = 250) {
  QElapsedTimer timer;
  timer.start();
  QEventLoop loop;

  while (timer.elapsed() < timeoutMs) {
    if (QSystemTrayIcon::isSystemTrayAvailable())
      return true;

    QTimer::singleShot(intervalMs, &loop, &QEventLoop::quit);
    loop.exec();
  }

  return false;
}

int main(int argc, char *argv[]) {

  QLockFile lock("/dev/shm/deskflow-control");
  if (!lock.tryLock()) {
    qWarning("another instance is already running ...");
    return 0;
  }

  QApplication app(argc, argv);

  if (!waitForSystemTray()) {
    qWarning("Please make sure your desktop environment supports tray icons ...");
  }

  QIcon iconImage(":/icon/deskflow-control.png");

  if (iconImage.isNull()) {
    qWarning("Tray icon not found!");
    return 1;
  }

  app.setWindowIcon(iconImage);
  QSystemTrayIcon *trayIcon = new QSystemTrayIcon(iconImage, &app);
  Viewer *viewer = new Viewer();
  QMenu *trayMenu = new QMenu();

  QAction *startAction = new QAction("Start");
  QAction *stopAction = new QAction("Stop");
  QAction *logAction = new QAction("Show");
  QAction *quitAction = new QAction("Quit");

  trayMenu->addAction(startAction);
  trayMenu->addAction(stopAction);
  trayMenu->addSeparator();
  trayMenu->addAction(logAction);
  trayMenu->addSeparator();
  trayMenu->addAction(quitAction);
  trayIcon->setContextMenu(trayMenu);

  auto updateActions = [&](bool isRunning) {
    startAction->setEnabled(!isRunning);
    stopAction->setEnabled(isRunning);
  };

  QObject::connect(viewer, &Viewer::visibilityChanged, [logAction](bool visible) {
    logAction->setText(visible ? "Hide" : "Show");
  });

  QObject::connect(logAction, &QAction::triggered, [viewer, logAction]() {
    if (viewer->isHidden()) {
      logAction->setText("Hide");
      viewer->show();
    } else {
      logAction->setText("Show");
      viewer->hide();
    }
  });

  auto runScript = [&](const QString &script) {
    deskflow = new QProcess();

    QObject::connect(deskflow, &QProcess::readyReadStandardOutput, [=]() {
      QString out = QString::fromLocal8Bit(deskflow->readAllStandardOutput()).trimmed();
      if (!out.isEmpty())
        viewer->appendLog(out, Qt::black);
    });

    QObject::connect(deskflow, &QProcess::readyReadStandardError, [=]() {
      QString err = QString::fromLocal8Bit(deskflow->readAllStandardError()).trimmed();
      if (!err.isEmpty())
        viewer->appendLog(err, Qt::red);
    });

    QObject::connect(deskflow, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     [=](int code, QProcess::ExitStatus) {
      if (code > 0) {
        viewer->appendLog(QString("[Process exited with code %1]").arg(code), Qt::red);
      }
      updateActions(false);
      if (deskflow) {
        deskflow->deleteLater();
        deskflow = nullptr;
      }
    });

    QObject::connect(deskflow, &QProcess::errorOccurred, [=](QProcess::ProcessError) {
      if (deskflow) {
        viewer->appendLog(QString("Process error: %1").arg(deskflow->errorString()), Qt::red);
        updateActions(false);
        deskflow->deleteLater();
        deskflow = nullptr;
      }
    });

    QObject::connect(deskflow, &QProcess::started, [=]() {
      updateActions(true);
    });
    deskflow->start(script, QStringList());
  };

  QString startScript = "/opt/deskflow-control/start";

  QFileInfo fiStart(startScript);
  if (fiStart.exists() && fiStart.isExecutable()) {
    QObject::connect(startAction, &QAction::triggered, [&]() {
      if (!deskflow) {
        runScript(startScript);
      }
    });

    QObject::connect(stopAction, &QAction::triggered, [&]() {
      if (deskflow) {
        deskflow->terminate();
        if (!deskflow->waitForFinished(3000)) {
          deskflow->kill();
        }
      }
    });
    runScript(startScript);
  } else {
    startAction->setEnabled(false);
    stopAction->setEnabled(false);
    viewer->appendLog(startScript + " not found", Qt::red);
  }


  QObject::connect(quitAction, &QAction::triggered, [&]() {
    if (deskflow) {
      deskflow->terminate();
      if (!deskflow->waitForFinished(3000)) {
        deskflow->kill();
      }
    }
   app.quit();
  });
  trayIcon->show();
  return app.exec();
}
