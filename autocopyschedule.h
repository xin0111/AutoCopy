#ifndef AUTOCOPYSCHEDULE_H
#define AUTOCOPYSCHEDULE_H

#include <QThread>
#include <QStringList>
#include <QHash>
#include <QSet>
#include "BlockingQueue.h"
#include "autocopy.h"


class QFileSystemWatcher;
class AutoRuleModel;
class QRunnable;
class AutoCopySchedule : public QThread
{
	Q_OBJECT
public:
	enum emTaskType { COPYFILEINIT, COPYFILETASK, UPDATEDIRECTORYTASK };
	AutoCopySchedule( AutoRuleModel* model);
public:
	void createWatcher();
	void copyExist();
	void addWatcher(const QString& source);
	//导入xml
	AutoCopyPropertyList importFileRules(const QString& filePath);
	AutoCopyPropertyList importRulesBat(const QString& filePath);
	//导出xml
	void exportFileRules(const QString& filePath, const AutoCopyPropertyList& rules);
	void exportRulesBat(const QString& filePath, const AutoCopyPropertyList& rules);
	//
	void copyFileTask(const QString& filePath, emTaskType eType);
	void copyFile(const QString& from);
	void updateDirFilesWatcher(const QString& root);
	QStringList checkCopyFile(const QString& from);
	//重置
	void resetSchedule();
	QStringList currentWatchPath();

signals:
	void sig_copyMsg(const QString& msg);
	void sig_errorMsg(const QString& error);
	void sig_tipMessage(const QString& error);
protected:
	void run();
private slots:
	void fileUpdated(const QString& file);
	void directoryUpdated(const QString &path);
private:
	QFileSystemWatcher* m_fileSysWatcher;
	AutoRuleModel* m_model;
	BlockingQueue<QRunnable*> m_tasksQueue;
	QStringList m_fileOnlyPaths;
	QStringList m_filesInOnlyPath;
};

#endif // AUTOCOPYSCHEDULE_H
