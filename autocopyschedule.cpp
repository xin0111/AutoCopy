#include "autocopyschedule.h"
#include <QFileSystemWatcher>
#include <QRunnable>
#include <QFile>
#include <QDomDocument>
#include <QThreadPool>
#include <QDebug>
#include <QDir>
#include <fstream>
#include <sstream>

#include "autocopy.h"
#include "autoruleview.h"
#include "Tools.h"

class CopyTask :public QRunnable
{
public:
	CopyTask(AutoCopySchedule* copyThread, const QString& from, AutoCopySchedule::emTaskType eType)
		:m_copyThread(copyThread), m_from(from), m_taskType(eType){}
	~CopyTask(){}
protected:
	virtual void run(){
		switch (m_taskType)
		{
		case AutoCopySchedule::COPYFILEINIT:
			m_copyThread->copyExist();
			break;
		case AutoCopySchedule::COPYFILETASK:
			m_copyThread->copyFile(m_from);
			break;
		case AutoCopySchedule::UPDATEDIRECTORYTASK:
			m_copyThread->updateDirFilesWatcher(m_from);
			break;
		default:
			break;
		}
	}
private:
	AutoCopySchedule* m_copyThread;
	QString  m_from;
	AutoCopySchedule::emTaskType m_taskType;
};

AutoCopySchedule::AutoCopySchedule(AutoRuleModel* model) :
m_model(model),
m_fileSysWatcher(nullptr)
{
	this->start();
}


void AutoCopySchedule::createWatcher()
{	
	if (m_fileSysWatcher)
	{
		sig_copyMsg("Auto Copy Working...");
		return;
	}
	m_fileSysWatcher = new QFileSystemWatcher(this);
	connect(m_fileSysWatcher, SIGNAL(directoryChanged(const QString &)), this, SLOT(directoryUpdated(const QString &)));
	connect(m_fileSysWatcher, SIGNAL(fileChanged(const QString &)), this, SLOT(fileUpdated(const QString &)));

	copyFileTask("", COPYFILEINIT);
}

void AutoCopySchedule::copyExist()
{
	AutoCopyPropertyList& rules = m_model->properties();
	AutoCopyPropertyList fileRules;
	int nAuto = rules.size();
	QString src; 
	//添加选择文件
	for (int i = 0; i < nAuto; i++)
	{
		const AutoCopyProperty& pro = rules.at(i);
		src = pro.Key;
		
		QFileInfo srcInfo(src);		
		if (srcInfo.isDir())
		{
			// remove contains
			if (m_fileOnlyPaths.contains(src))
				m_fileOnlyPaths.removeOne(src);
		}
		else 
		{
			QString srcPath = srcInfo.absolutePath();
			//file only 
			m_fileOnlyPaths.push_back(srcPath);
			m_filesInOnlyPath.push_back(src);
		}		
	}
	//添加监视
	for (int i = 0; i < nAuto; i++)
	{
		const AutoCopyProperty& pro = rules.at(i);
		src = pro.Key;
		
		addWatcher(src);
	
		//跳过已拷贝
		if (pro.Advanced)
			continue;

		copyFile(src);
	}
}

void AutoCopySchedule::addWatcher(const QString& source)
{	
	QFileInfo srcInfo(source);
	if (srcInfo.isDir())
	{
		QStringList dirInWatcher = m_fileSysWatcher->directories();
		//子文件夹
		updateDirFilesWatcher(source);
		//当前文件夹		
		m_fileSysWatcher->addPath(source);
		
	}
	else 
	{
		QString srcPath = QFileInfo(source).absolutePath();		
		//父文件夹		
		m_fileSysWatcher->addPath(srcPath);
		//当前文件		
		m_fileSysWatcher->addPath(source);
	}
}

AutoCopyPropertyList AutoCopySchedule::importFileRules(const QString& filePath)
{
	AutoCopyPropertyList rules;
	QDomDocument doc;
	CTools::openXml(doc, filePath);
	QDomElement root = doc.firstChildElement("Auto");
	QDomNodeList ruleNodes = root.childNodes();
	if (ruleNodes.isEmpty()) return rules;
	for (int rule = 0; rule < ruleNodes.size(); rule++)
	{		
		AutoCopyProperty prop;
		prop.Key = ruleNodes.at(rule).toElement().attribute("src");
		prop.KeyType = AutoCopyProperty::FILE_PATH;
		prop.Value = ruleNodes.at(rule).toElement().attribute("dest");
		prop.ValueType = AutoCopyProperty::PATH;
		rules << prop;
	}
	return rules;
}

AutoCopyPropertyList AutoCopySchedule::importRulesBat(const QString& filePath)
{
	AutoCopyPropertyList rules;
	
	std::fstream fin(filePath.toLocal8Bit().toStdString());
	std::string strLine;
	while (getline(fin,strLine))
	{
		QString qstrLine = QString::fromStdString(strLine);
		if (qstrLine.startsWith("copy"))
		{
			QStringList pathInfos = qstrLine.split(" ", QString::SkipEmptyParts);
			if (pathInfos.size() < 3 ) continue;
			AutoCopyProperty prop;
			prop.Key = pathInfos[1];
			prop.KeyType = AutoCopyProperty::FILE_PATH;
			prop.Value = pathInfos[2];
			prop.ValueType = AutoCopyProperty::PATH;
			rules << prop;
		}
	}
	fin.close();
	return rules;
}

void AutoCopySchedule::exportFileRules(const QString& filePath, const AutoCopyPropertyList& rules)
{
	QDomDocument doc;
	QDomElement root = doc.createElement("Auto");
	int nAuto = rules.size();
	for (int i = 0; i < nAuto; i++)
	{
		QDomElement node = doc.createElement("Rule");	
		node.setAttribute("src", rules.at(i).Key);
		node.setAttribute("dest", rules.at(i).Value.toString());
		root.appendChild(node);
	}
	doc.appendChild(root);
	CTools::saveXml(doc, filePath);
	sig_tipMessage(QString::fromLocal8Bit("导出完成."));
}

void AutoCopySchedule::exportRulesBat(const QString& filePath, const AutoCopyPropertyList& rules)
{
	QFile file(filePath);
	if (file.open(QIODevice::WriteOnly))
	{
		QTextStream out(&file);
		int nAuto = rules.size();
		for (int i = 0; i < nAuto; i++)
		{
			QString rule = QString("copy %1 %2").arg(rules.at(i).Key).arg(rules.at(i).Value.toString());
			out << rule  << "\n";			
		}
		file.close();
		sig_tipMessage(QString::fromLocal8Bit("导出完成."));
	}	
}

void AutoCopySchedule::updateDirFilesWatcher(const QString& root)
{
	//如果文件已删除，需要重新添加
	const QDir dir(root);	
	//file only
	QFileInfoList firstEntryList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Modified);
	QString filePath;
	QStringList fileInWatcher = m_fileSysWatcher->files();
	for each (const QFileInfo& info in firstEntryList)
	{
		filePath = info.filePath();

		if (m_fileOnlyPaths.contains(root))
		{
			if (!m_filesInOnlyPath.contains(filePath))
			{
				qDebug() << "not selected " << filePath;
				continue;
			}
		}

		if (fileInWatcher.contains(filePath) )
		{
			qDebug() << "watcher contains " << filePath;
			continue;
		}	

		copyFile(filePath);

		addWatcher(filePath);
	}
}

void AutoCopySchedule::copyFileTask(const QString& filePath, emTaskType eType/*=COPYFILETASK*/)
{
	if (QFile::exists(filePath) || eType == COPYFILEINIT)
	{
		CopyTask *copyTask = new CopyTask(this, filePath, eType);
		m_tasksQueue.put(copyTask);
	}
}

void AutoCopySchedule::copyFile(const QString& from)
{
	const QStringList& dest = checkCopyFile(from);
	QFile file(from);	
	for (int i = 0; i < dest.size();i++)
	{
		auto aaa = dest.at(i);
		if (!dest.at(i).isEmpty() && QFile::exists(from) )
		{
			QString strMsg = QString("Copy %1 \n\t to %2").
				arg(from).arg(dest.at(i));
			QString error;
			if (!CTools::copyFileToPath(from, dest.at(i), error))
			{
				emit sig_errorMsg(strMsg + QString("  failed : %3").arg(error));
			}
			else
			{
				emit sig_copyMsg(strMsg);
			}
		}
	}
}

QStringList AutoCopySchedule::checkCopyFile(const QString& from)
{
	AutoCopyPropertyList& propList = m_model->properties();
	QStringList copyToDirs;
	for each (const AutoCopyProperty& var in propList)
	{
		if (QRegExp(var.Key + ".*").exactMatch(from))
		{
			QFileInfo keyInfo(var.Key);
			QString keyPath = keyInfo.isDir() ? var.Key : keyInfo.absolutePath();
			if (keyPath == from)				
				copyToDirs.push_back(var.Value.toString());
			QFileInfo fromInfo(from);
			QString fromPath = fromInfo.isDir() ? from : fromInfo.absolutePath();
			
			copyToDirs.push_back(var.Value.toString());
		}
	}
	return copyToDirs;
}

void AutoCopySchedule::resetSchedule()
{
	QStringList paths;
	if (m_fileSysWatcher)
	{
		disconnect(m_fileSysWatcher, SIGNAL(directoryChanged(const QString &)), this, SLOT(directoryUpdated(const QString &)));
		disconnect(m_fileSysWatcher, SIGNAL(fileChanged(const QString &)), this, SLOT(fileUpdated(const QString &)));
		delete m_fileSysWatcher;
		m_fileSysWatcher = nullptr;
	}
	m_fileOnlyPaths.clear();
	m_filesInOnlyPath.clear();
	m_tasksQueue.clear();
}

QStringList AutoCopySchedule::currentWatchPath()
{
	QStringList paths;
	paths << m_fileSysWatcher->files();
	paths << m_fileSysWatcher->directories();
	return paths;
}

void AutoCopySchedule::run()
{
	while (true)
	{
		if (!m_tasksQueue.isEmpty())
		{
			QRunnable *task = m_tasksQueue.take();
			task->run();
		}
	}
}

void AutoCopySchedule::directoryUpdated(const QString &path)
{
	qDebug() << "dir" << path;
	copyFileTask(path, UPDATEDIRECTORYTASK);
}

void AutoCopySchedule::fileUpdated(const QString& file)
{
	qDebug() << "file" << file;
	copyFileTask(file, COPYFILETASK);
}


