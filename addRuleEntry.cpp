#include "addRuleEntry.h"

#include <QCompleter>
#include <QMetaProperty>
#include "editwidgets.h"
#include "CopyPage.h"
#include "Tools.h"

AddRuleEntry::AddRuleEntry(QDialog* p)
: QDialog(p)
{
  this->setupUi(this);
  this->tabWidget_rule->setAcceptDrops(true);
  this->tabWidget_rule->installEventFilter(this);
  QObject::connect(this->buttonBox, &QDialogButtonBox::accepted, [=]{  
	  saveCopyRule();
	  accept();
  });
}


void AddRuleEntry::on_btn_newPage_clicked()
{
	addNewPage();
}

void AddRuleEntry::on_btn_delPage_clicked()
{
	int nTabIndex = this->tabWidget_rule->currentIndex();
	this->tabWidget_rule->removeTab(nTabIndex);
	if (this->tabWidget_rule->count() == 0)
	{
		resetPage();
	}
}

void AddRuleEntry::resetPage()
{
	addNewPage();
}

void AddRuleEntry::addNewPage()
{
	CopyPage* pPage = new CopyPage(this);

	int nCount = this->tabWidget_rule->count();
	QStringList pagesText;
	for (int i = 0; i < nCount;++i)
	{
		pagesText << this->tabWidget_rule->tabText(i);
	}	
	int nIndex = CTools::CalcNextIndex(nCount, pagesText);

	int addIndex = this->tabWidget_rule->addTab(pPage,
		QString::fromLocal8Bit("¹æÔòÒ³%1").arg(nIndex));
	this->tabWidget_rule->setCurrentIndex(addIndex);
}

bool AddRuleEntry::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == this->tabWidget_rule)
	{
		if (event->type() == QEvent::DragEnter)
		{
			event->accept();
		}
		if (event->type() == QEvent::DragMove)
		{
			int nIndex = this->tabWidget_rule->tabBar()->tabAt(((QDragMoveEvent*)event)->pos());
			if (nIndex != -1)
			{
				this->tabWidget_rule->tabBar()->setCurrentIndex(nIndex);
			}
			event->accept();
		}
	}
	return QDialog::eventFilter(watched, event);
}


void AddRuleEntry::saveCopyRule()
{
	int nRule = this->tabWidget_rule->count();
	if (nRule == 0) return;
	CopyPage * rulePage = NULL;
	QStringList fromList, toList;

	for (int i = 0; i < nRule; i++)
	{
		rulePage = (CopyPage*)this->tabWidget_rule->widget(i);
		if (rulePage)
		{
			if (!rulePage->GetRuleEnable()) continue;
			fromList = rulePage->CopyFormFilePath();
			toList = rulePage->CopyToPath();
			if (toList.isEmpty())
				continue;

			for each (QString var in fromList)
			{
				m_fileHash[var] = toList;
			}
		}
	}
}
