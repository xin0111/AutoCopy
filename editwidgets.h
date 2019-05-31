#ifndef EIDTWIDGETS_H__
#define EIDTWIDGETS_H__

#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>

class QToolButton;
class QFileDialog;

// common widgets for Qt based CMake

/// Editor widget for editing paths or file paths
class RuleFileEditor : public QLineEdit
{
  Q_OBJECT
public:
  RuleFileEditor(QWidget* p, const QString& var);
protected slots:
  virtual void chooseFile() = 0;
signals:
  void fileDialogExists(bool);

protected:
  void resizeEvent(QResizeEvent* e);
  QToolButton* ToolButton;
  QString Variable;
};

/// editor widget for editing files
class RulePathEditor : public RuleFileEditor
{
  Q_OBJECT
public:
  RulePathEditor(QWidget* p = nullptr, const QString& var = QString());
  void chooseFile();
};

/// editor widget for editing paths
class RuleFilePathEditor : public RuleFileEditor
{
  Q_OBJECT
public:
  RuleFilePathEditor(QWidget* p = nullptr, const QString& var = QString());
  void chooseFile();
  void setPathEnable(bool bEnable){ m_pathEnable = bEnable; }
private:
  QFileDialog* m_dlg;
  QString m_selectedPath;
  bool  m_pathEnable;//ÊÇ·ñÄÜÂ·¾¶
};

/// completer class that returns native cmake paths
class RuleFileCompleter : public QCompleter
{
  Q_OBJECT
public:
  RuleFileCompleter(QObject* o, bool dirs);
  virtual QString pathFromIndex(const QModelIndex& idx) const;
};

// editor for strings
class RuleComboBox : public QComboBox
{
  Q_OBJECT
  Q_PROPERTY(QString value READ currentText WRITE setValue USER true);

public:
  RuleComboBox(QWidget* p, QStringList strings)
    : QComboBox(p)
  {
    this->addItems(strings);
  }
  void setValue(const QString& v)
  {
    int i = this->findText(v);
    if (i != -1) {
      this->setCurrentIndex(i);
    }
  }
};

#endif // eidtwidgets_H__
