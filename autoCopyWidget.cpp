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
#include <QCloseEvent>

#include "Tools.h"
#include "autocopyschedule.h"
#include "autocopy.h"



AutoCopyWidget::AutoCopyWidget(QWidget *parent)
: QWidget(parent),
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

	//托盘
	enableTrayIcon();

	// copy 任务
	m_copySchedule = new AutoCopySchedule(ui.RuleValues->cacheModel());
	connect(m_copySchedule, SIGNAL(sig_tipMessage(const QString&)), this, SLOT(tipMessage(const QString&)));
	connect(m_copySchedule, SIGNAL(sig_copyMsg(const QString&)), this, SLOT(displayCopyMsg(const QString&)));
	connect(m_copySchedule, SIGNAL(sig_errorMsg(const QString&)), this, SLOT(displayErrorMsg(const QString&)));	

	QObject::connect(ui.Search, SIGNAL(textChanged(QString)), this,
		SLOT(setSearchFilter(QString)));

	ui.groupedCheck->setVisible(false);
	setAdvancedView(true);

	connect(ui.RuleValues, &AutoRuleView::sig_addEditedTask, [=](const QString& key)
	{
		Q_UNUSED(key);
		m_copySchedule->resetSchedule();
		ui.btn_Start->setText(QString::fromLocal8Bit("开始"));
		ui.btn_Check->setEnabled(false);
	});
	connect(ui.RuleValues, &AutoRuleView::sig_updateSchedule, [=]()
	{
		m_copySchedule->resetSchedule();
		ui.btn_Start->setText(QString::fromLocal8Bit("开始"));
		ui.btn_Check->setEnabled(false);
	});
	ui.btn_Check->setEnabled(false);
}

AutoCopyWidget::~AutoCopyWidget()
{
	QSettings settings("AutoCopy","Settings");
	settings.beginGroup("StartPath");
	settings.setValue("geometry", QVariant(saveGeometry()));
	settings.setValue("SplitterSizes", ui.splitter->saveState());
}

void AutoCopyWidget::on_btn_Import_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open File"), "", tr("AutoCopy Files (*.xml | *.bat )"));
	if (!fileName.isEmpty())
	{
		resetDisplay();
		AutoCopyPropertyList& rules = fileName.endsWith("xml") ?
			m_copySchedule->importFileRules(fileName) : m_copySchedule->importRulesBat(fileName);
		AutoRuleModel* m = ui.RuleValues->cacheModel();
		for each (AutoCopyProperty var in rules)
		{
			m->insertProperty(var.KeyType, var.ValueType, var.Key.replace("\\", "/"),
				var.Key, var.Value.toString().replace("\\", "/"),
				false);
		}

		this->setWindowTitle(QFileInfo(fileName).baseName() + " - " + m_baseTitle);
		
	}
}

void AutoCopyWidget::on_btn_Export_clicked()
{
	QString filePath = QFileDialog::getSaveFileName(this, QString::fromLocal8Bit("保存文件"),
		"AutoCopy", "AutoCopy Files (*.bat | *.xml)");
	if (!filePath.isEmpty())
	{
		AutoRuleModel* m = ui.RuleValues->cacheModel();
		AutoCopyPropertyList& propertyList = m->properties();

		filePath.endsWith("xml")?
		m_copySchedule->exportFileRules(filePath, propertyList):
		m_copySchedule->exportRulesBat(filePath,propertyList);

		this->setWindowTitle(QFileInfo(filePath).baseName() + " - " + m_baseTitle);
	}
}

void AutoCopyWidget::on_btn_ClearOutPut_clicked()
{
	ui.Output->clear();
}

void AutoCopyWidget::on_btn_Check_clicked()
{
	QStringList& paths = m_copySchedule->currentWatchPath();
	for each (QString var in paths)
	{
		displayCopyMsg(var);
	}
}

void AutoCopyWidget::on_btn_Start_clicked()
{
	m_copySchedule->createWatcher();
	ui.btn_Start->setText(QString::fromLocal8Bit("Working..."));
	ui.btn_Check->setEnabled(true);
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

void AutoCopyWidget::setAdvancedView(bool v)
{
	ui.RuleValues->setShowAdvanced(v);
}

void AutoCopyWidget::resetDisplay()
{
	m_copySchedule->resetSchedule();
	ui.btn_Start->setText(QString::fromLocal8Bit("开始"));
	ui.btn_Check->setEnabled(false);
	AutoRuleModel* m = ui.RuleValues->cacheModel();
	m->clear();
	this->setWindowTitle(m_baseTitle);
}

void AutoCopyWidget::enableTrayIcon()
{
	QAction *quitAction = new QAction(tr("&Quit"), this);
	connect(quitAction, &QAction::triggered, qApp,
		&QCoreApplication::quit);
	QMenu *trayIconMenu = new QMenu(this);
	trayIconMenu->addAction(quitAction);

	m_trayIcon = new QSystemTrayIcon(this);
	m_trayIcon->setToolTip(QString::fromLocal8Bit("AutoCopy"));
	m_trayIcon->setContextMenu(trayIconMenu);
	QIcon trayicon = QIcon::fromTheme("AutoCopy-tray", QIcon(":images/autocopy.png"));
	m_trayIcon->setIcon(trayicon);
	auto trayIconActivated = [this](QSystemTrayIcon::ActivationReason r){
		if (r == QSystemTrayIcon::DoubleClick) {
			this->activateWindow();
			this->showNormal();
		}
	};
	connect(m_trayIcon, &QSystemTrayIcon::activated, this, trayIconActivated);
	m_trayIcon->show();
}

void AutoCopyWidget::changeEvent(QEvent *e)
{
	//if (e->type() == QEvent::WindowStateChange && this->isMinimized())
	//{//最小化隐藏
	//	this->hide();
	//}
}

void AutoCopyWidget::closeEvent(QCloseEvent *event)
{
	event->ignore();
	this->hide();
}
