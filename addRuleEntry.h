#ifndef AddRuleEntry_h
#define AddRuleEntry_h

#include <QCheckBox>
#include <QStringList>
#include <QDialog>
#include "autocopy.h"

#include "ui_AddRuleEntry.h"

class AddRuleEntry
	: public QDialog
  , public Ui::AddRuleEntry
{
  Q_OBJECT
public:
	AddRuleEntry(QDialog* p = Q_NULLPTR);
	QHash<QString, QStringList>& getCopyRule(){ return m_fileHash; }

	void saveCopyRule();
protected slots:
	void on_btn_delPage_clicked();
	void on_btn_newPage_clicked();
private:
	void resetPage();
	void addNewPage();
protected:
	bool eventFilter(QObject *watched, QEvent *event);
private:
	QHash<QString, QStringList> m_fileHash;
};

#endif
