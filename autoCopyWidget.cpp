#include "autoCopyWidget.h"
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QMenu>
#include <QInputDialog>
#include <QDebug>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QShortcut>
#include <QTime>
#include <QSettings>
#include "Tools.h"
#include "autocopyschedule.h"
#include "addRuleEntry.h"
#include "autocopy.h"


AutoCopyWidget::AutoCopyWidget(QWidget *parent)
: QWidget(parent),
m_fileWatcher(new QFileSystemWatcher(this)),
m_directoryWatcher(new QFileSystemWatcher(this)),
	m_bWatching(false)
{
	ui.setupUi(this);
	QSettings settings("AutoCopy", "Settings");
	settings.beginGroup("StartPath");
	restoreGeometry(settings.value("geometry").toByteArray());
	QByteArray p = settings.value("SplitterSizes").toByteArray();
	ui.splitter->restoreState(p);

	m_baseTitle = this->windowTitle();
	{
		m_OutPutMenu = new QMenu(this);
		
		m_OutPutMenu->addSeparator();
		m_OutPutMenu->addAction(tr("Find..."), this, SLOT(doOutputFindDialog()), QKeySequence::Find);
		new QShortcut(QKeySequence(QKeySequence::Find), this,
			SLOT(doOutputFindDialog()));
		m_OutPutMenu->addAction(tr("Find Next"), this, SLOT(doOutputFindNext()), QKeySequence::FindNext);
		new QShortcut(QKeySequence(QKeySequence::FindNext), this,
			SLOT(doOutputFindNext()));
		m_OutPutMenu->addAction(tr("Find Previous"), this, SLOT(doOutputFindPrev()), QKeySequence::FindPrevious);
		new QShortcut(QKeySequence(QKeySequence::FindPrevious), this,
			SLOT(doOutputFindPrev()));
		m_OutPutMenu->addSeparator();
		m_OutPutMenu->addAction(tr("Goto Next Error"), this, SLOT(doOutputErrorNext()), Qt::Key_F8);
		new QShortcut(QKeySequence(Qt::Key_F8), this,
			SLOT(doOutputErrorNext()));
	}

	this->ErrorFormat.setForeground(QBrush(Qt::red));
	ui.Output->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.Output, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(doOutputContextMenu(const QPoint&)));
	m_copySchedule = new AutoCopySchedule(m_fileWatcher, m_directoryWatcher, ui.RuleValues->cacheModel());
	connect(m_copySchedule, SIGNAL(sig_tipMessage(const QString&)), this, SLOT(tipMessage(const QString&)));
	connect(m_copySchedule, SIGNAL(sig_copyMsg(const QString&)), this, SLOT(displayCopyMsg(const QString&)));
	connect(m_copySchedule, SIGNAL(sig_errorMsg(const QString&)), this, SLOT(displayErrorMsg(const QString&)));	

	QObject::connect(ui.Search, SIGNAL(textChanged(QString)), this,
		SLOT(setSearchFilter(QString)));
	//QObject::connect(ui.groupedCheck, SIGNAL(toggled(bool)), this,
	//	SLOT(setGroupedView(bool)));
	//QObject::connect(ui.groupedCheck, SIGNAL(toggled(bool)), this,
	//	SLOT(setAdvancedView(bool)));
	ui.groupedCheck->setVisible(false);
	setAdvancedView(true);
	// start watching
	m_bWatching = true;
	connect(m_directoryWatcher, SIGNAL(directoryChanged(const QString &)), this, SLOT(directoryUpdated(const QString &)));
	connect(m_fileWatcher, SIGNAL(fileChanged(const QString &)), this, SLOT(fileUpdated(const QString &)));

	m_copySchedule->start();
}

AutoCopyWidget::~AutoCopyWidget()
{
	QSettings settings("AutoCopy","Settings");
	settings.beginGroup("StartPath");
	settings.setValue("geometry", QVariant(saveGeometry()));
	settings.setValue("SplitterSizes", ui.splitter->saveState());
}

void AutoCopyWidget::on_btn_Edit_clicked()
{
	AddRuleEntry* w =
		new AddRuleEntry();

	if (QDialog::Accepted == w->exec()) {
		AutoRuleModel* m = ui.RuleValues->cacheModel();
		m->updatePropertyAdvance(); 
		QHash<QString, QStringList>& rules = w->getCopyRule();

		QHashIterator<QString, QStringList> i(rules);
		QString strCopyFrom;
		QStringList copyToList;
	
		while (i.hasNext()) {
			i.next();
			strCopyFrom = i.key();
			copyToList = i.value();			
			for each (const QString& var in copyToList)
			{
				m->insertProperty(AutoCopyProperty::FILEANDPATH, AutoCopyProperty::PATH, strCopyFrom, strCopyFrom, var,
					false);				
			}
		}
		if (!rules.isEmpty())
		{
			//m->chang
			m_copySchedule->copyFileTask("", AutoCopySchedule::COPYFILEINIT);
		}
	}
}

void AutoCopyWidget::on_btn_Import_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open File"), "", tr("Xml Files (*.xml )"));
	if (!fileName.isEmpty())
	{
		on_btn_Clear_clicked();
		AutoCopyPropertyList& rules = m_copySchedule->importFileRules(fileName);
		AutoRuleModel* m = ui.RuleValues->cacheModel();
		for each (AutoCopyProperty var in rules)
		{			
			m->insertProperty(var.KeyType, var.ValueType, var.Key.replace("\\", "/"), 
				var.Key, var.Value.toString().replace("\\", "/"),
				false);
		}	

		m_copySchedule->copyFileTask("", AutoCopySchedule::COPYFILEINIT);

		this->setWindowTitle(QFileInfo(fileName).baseName() + " - " + m_baseTitle);
	}
}

void AutoCopyWidget::on_btn_Export_clicked()
{
	QString filePath = QFileDialog::getSaveFileName(this, QString::fromLocal8Bit("保存文件"),
		"AutoCopy.xml", "Xml Files (*.xml )");
	if (!filePath.isEmpty())
	{
		AutoRuleModel* m = ui.RuleValues->cacheModel();
		AutoCopyPropertyList& propertyList = m->properties();
		m_copySchedule->exportFileRules(filePath, propertyList);

		this->setWindowTitle(QFileInfo(filePath).baseName() + " - " + m_baseTitle);
	}
}

void AutoCopyWidget::on_btn_ClearOutPut_clicked()
{
	ui.Output->clear();
}

void AutoCopyWidget::on_btn_Clear_clicked()
{
	m_copySchedule->reset();
	AutoRuleModel* m = ui.RuleValues->cacheModel();
	m->clear();
	this->setWindowTitle(m_baseTitle);
}

void AutoCopyWidget::on_btn_Check_clicked()
{
	QStringList& paths = m_copySchedule->currentWatchPath();
	for each (QString var in paths)
	{
		displayCopyMsg(var);
	}
}

void AutoCopyWidget::directoryUpdated(const QString &path)
{
	qDebug() << "dir" << path;
	m_copySchedule->copyFileTask(path, AutoCopySchedule::UPDATEDIRECTORYTASK);
}

void AutoCopyWidget::fileUpdated(const QString& file)
{
	qDebug() << "file" << file;
	m_copySchedule->copyFileTask(file);
}

void AutoCopyWidget::displayCopyMsg(const QString& msg)
{
	ui.Output->setCurrentCharFormat(this->MessageFormat);
	QTime time;
	QString strTime =  time.currentTime().toString();
	ui.Output->append(strTime + " " +msg);
}

void AutoCopyWidget::displayErrorMsg(const QString& msg)
{
	ui.Output->setCurrentCharFormat(this->ErrorFormat);
	QTime time;
	QString strTime = time.currentTime().toString();
	ui.Output->append(strTime + " " + msg);
}

void AutoCopyWidget::doOutputContextMenu(QPoint pt)
{
	m_OutPutMenu->exec(ui.Output->mapToGlobal(pt));
}


void AutoCopyWidget::doOutputFindDialog()
{
	QStringList strings(this->FindHistory);

	QString selection = ui.Output->textCursor().selectedText();
	if (!selection.isEmpty() && !selection.contains(QChar::ParagraphSeparator) &&
		!selection.contains(QChar::LineSeparator)) {
		strings.push_front(selection);
	}

	bool ok;
	QString search = QInputDialog::getItem(this, tr("Find in Output"),
		tr("Find:"), strings, 0, true, &ok);
	if (ok && !search.isEmpty()) {
		if (!this->FindHistory.contains(search)) {
			this->FindHistory.push_front(search);
		}
		doOutputFindNext();
	}
}

void AutoCopyWidget::doOutputFindPrev()
{
	doOutputFindNext(false);
}

void AutoCopyWidget::doOutputFindNext(bool directionForward)
{
	if (this->FindHistory.isEmpty()) {
		doOutputFindDialog(); // will re-call this function again
		return;
	}

	QString search = this->FindHistory.front();

	QTextCursor textCursor = ui.Output->textCursor();
	QTextDocument* document = ui.Output->document();
	QTextDocument::FindFlags flags;
	if (!directionForward) {
		flags |= QTextDocument::FindBackward;
	}

	textCursor = document->find(search, textCursor, flags);

	if (textCursor.isNull()) {
		// first search found nothing, wrap around and search again
		textCursor = ui.Output->textCursor();
		textCursor.movePosition(directionForward ? QTextCursor::Start
			: QTextCursor::End);
		textCursor = document->find(search, textCursor, flags);
	}

	if (textCursor.hasSelection()) {
		ui.Output->setTextCursor(textCursor);
	}
}

void AutoCopyWidget::doOutputErrorNext()
{
	QTextCursor textCursor = ui.Output->textCursor();
	bool atEnd = false;

	// move cursor out of current error-block
	if (textCursor.blockCharFormat() == this->ErrorFormat) {
		atEnd = !textCursor.movePosition(QTextCursor::NextBlock);
	}

	// move cursor to next error-block
	while (textCursor.blockCharFormat() != this->ErrorFormat && !atEnd) {
		atEnd = !textCursor.movePosition(QTextCursor::NextBlock);
	}

	if (atEnd) {
		// first search found nothing, wrap around and search again
		atEnd = !textCursor.movePosition(QTextCursor::Start);

		// move cursor to next error-block
		while (textCursor.blockCharFormat() != this->ErrorFormat && !atEnd) {
			atEnd = !textCursor.movePosition(QTextCursor::NextBlock);
		}
	}

	if (!atEnd) {
		textCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

		QTextCharFormat selectionFormat;
		selectionFormat.setBackground(Qt::yellow);
		QTextEdit::ExtraSelection extraSelection = { textCursor, selectionFormat };
		ui.Output->setExtraSelections(QList<QTextEdit::ExtraSelection>()
			<< extraSelection);

		// make the whole error-block visible
		ui.Output->setTextCursor(textCursor);

		// remove the selection to see the extraSelection
		textCursor.setPosition(textCursor.anchor());
		ui.Output->setTextCursor(textCursor);
	}
}

void AutoCopyWidget::tipMessage(const QString& msg)
{
	QMessageBox::information(this, QString::fromLocal8Bit("提示"), msg);
}


void AutoCopyWidget::setSearchFilter(const QString& str)
{
	ui.RuleValues->selectionModel()->clear();
	ui.RuleValues->setSearchFilter(str);
}

void AutoCopyWidget::setGroupedView(bool v)
{
	ui.RuleValues->cacheModel()->setViewType(v ? AutoRuleModel::GroupView
		: AutoRuleModel::FlatView);
	ui.RuleValues->setRootIsDecorated(v);
}

void AutoCopyWidget::setAdvancedView(bool v)
{
	ui.RuleValues->setShowAdvanced(v);
}