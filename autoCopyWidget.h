#ifndef AUTOCOPY_H
#define AUTOCOPY_H

#include <QWidget>
#include <QObject>
#include <QStringList>
#include <QPointer>
#include <QSystemTrayIcon>

#include "ui_autoCopyWidget.h"


class AutoCopySchedule;
class QFileSystemWatcher;

class AutoCopyWidget : public QWidget
{
	Q_OBJECT
public:
	AutoCopyWidget(QWidget *parent = 0);
	~AutoCopyWidget();
	//мпелотй╬
	void enableTrayIcon();
public slots:
	//
	void on_btn_Import_clicked();
	void on_btn_Export_clicked();
	void on_btn_ClearOutPut_clicked();
	void on_btn_Check_clicked();
	void on_btn_Start_clicked();
	//
	void tipMessage(const QString& msg);
	void displayCopyMsg(const QString& msg);
	void displayErrorMsg(const QString& msg);
signals:
	void sig_switchShow();
private slots:
	void doOutputContextMenu(QPoint pt);
	void doOutputFindDialog();
	void doOutputFindPrev();
	void doOutputFindNext(bool directionForward = true);
	void doOutputErrorNext();
	void setSearchFilter(const QString& str);
	void setAdvancedView(bool v);
	void resetDisplay();
protected:
	void changeEvent(QEvent *) override;
private:
	QTextCharFormat ErrorFormat;
	QTextCharFormat MessageFormat;
	QStringList FindHistory;
	AutoCopySchedule* m_copySchedule;
private:
	Ui::AutoCopyWidget ui;
	QMenu* m_OutPutMenu;
	bool m_bWatching;
	QString m_baseTitle;
	QPointer<QSystemTrayIcon> m_trayIcon;
};

#endif // AUTOCOPY_H
