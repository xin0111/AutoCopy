#ifndef AUTOCOPY_H
#define AUTOCOPY_H

#include <QWidget>
#include <QObject>
#include <QStringList>
#include "ui_autoCopyWidget.h"


class AutoCopySchedule;
class QFileSystemWatcher;

class AutoCopyWidget : public QWidget
{
	Q_OBJECT
public:
	AutoCopyWidget(QWidget *parent = 0);
	~AutoCopyWidget();
public slots:
	//
	void directoryUpdated(const QString &path);
	void fileUpdated(const QString& file);
	//
	void on_btn_Edit_clicked();
	void on_btn_Import_clicked();
	void on_btn_Export_clicked();
	void on_btn_ClearOutPut_clicked();
	void on_btn_Clear_clicked();
	void on_btn_Check_clicked();
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
	void setGroupedView(bool v);
	void setAdvancedView(bool v);
private:
	QFileSystemWatcher* m_fileWatcher;
	QFileSystemWatcher* m_directoryWatcher;
	QTextCharFormat ErrorFormat;
	QTextCharFormat MessageFormat;
	QStringList FindHistory;
	AutoCopySchedule* m_copySchedule;
private:
	Ui::AutoCopyWidget ui;
	QMenu* m_OutPutMenu;
	QAction* m_test;
	bool m_bWatching;
	QString m_baseTitle;
};

#endif // AUTOCOPY_H