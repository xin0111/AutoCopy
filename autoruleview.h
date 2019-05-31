/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef QCMakeCacheView_h
#define QCMakeCacheView_h

#include <QItemDelegate>
#include <QSet>
#include <QStandardItemModel>
#include <QTreeView>

#include "autocopy.h"

class QSortFilterProxyModel;
class AutoRuleModel;
class RuleAdvancedFilter;

/// Qt view class for cache properties
class AutoRuleView : public QTreeView
{
  Q_OBJECT
public:
  AutoRuleView(QWidget* p);

  // retrieve the QCMakeCacheModel storing all the pointers
  // this isn't necessarily the model one would get from model()
  AutoRuleModel* cacheModel() const;

  // get whether to show advanced entries
  bool showAdvanced() const;

  QSize sizeHint() const { return QSize(200, 200); }

protected:
	void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
public slots:
  // set whether to show advanced entries
  void setShowAdvanced(bool);
  // set the search filter string.  any property key or value not matching will
  // be filtered out
  void setSearchFilter(const QString&);
signals:
  void sig_addEditedTask(const QString&);
  void sig_updateSchedule();
protected:
  QModelIndex moveCursor(CursorAction, Qt::KeyboardModifiers);
  bool event(QEvent* e);
  AutoRuleModel* CacheModel;
  RuleAdvancedFilter* AdvancedFilter;
  QSortFilterProxyModel* SearchFilter;
  QMenu* m_pMenu;
};

/// Qt model class for cache properties
class AutoRuleModel : public QStandardItemModel
{
  Q_OBJECT
public:
  AutoRuleModel(QObject* parent);
  ~AutoRuleModel();

  // roles used to retrieve extra data such has help strings, types of
  // properties, and the advanced flag
  enum
  {
    HelpRole = Qt::ToolTipRole,
	KeyTypeRole = Qt::UserRole,
    ValueTypeRole,
    AdvancedRole,
    StringsRole,
    GroupRole
  };

public slots:

  // set whether to show new properties in red
  void setShowNewProperties(bool);

  // clear everything from the model
  void clear();

  // set flag whether the model can currently be edited.
  void setEditEnabled(bool);

  // insert a new property at a row specifying all the information about the
  // property
  bool insertProperty(AutoCopyProperty::PropertyType keyt, 
					  AutoCopyProperty::PropertyType valuet, const QString& name,
                      const QString& description, const QVariant& value,
                      bool advanced);
public:
  // get the properties
	AutoCopyPropertyList properties() const;

  // editing enabled
  bool editEnabled() const;

  // returns how many new properties there are
  int newPropertyCount() const;

  // return flags (overloaded to modify flag based on EditEnabled flag)
  Qt::ItemFlags flags(const QModelIndex& index) const;
  QModelIndex buddy(const QModelIndex& idx) const;

  // get the data in the model for this property
  void getPropertyData(const QModelIndex& idx1, AutoCopyProperty& prop) const;

  void updatePropertyAdvance();
protected:
  bool EditEnabled;
  int NewPropertyCount;
  bool ShowNewProperties;

  // set the data in the model for this property
  void setPropertyData(const QModelIndex& idx1, const AutoCopyProperty& p,
                       bool isNew);

  // breaks up he property list into groups
  // where each group has the same prefix up to the first underscore
  static void breakProperties(const QSet<AutoCopyProperty>& props,
	  QMap<QString, AutoCopyPropertyList>& result);

  // gets the prefix of a string up to the first _
  static QString prefix(const QString& s);
};

/// Qt delegate class for interaction (or other customization)
/// with cache properties
class AutoRuleModelDelegate : public QItemDelegate
{
  Q_OBJECT
public:
  AutoRuleModelDelegate(QObject* p);
  /// create our own editors for cache properties
  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const;
  bool editorEvent(QEvent* event, QAbstractItemModel* model,
                   const QStyleOptionViewItem& option,
                   const QModelIndex& index);
  bool eventFilter(QObject* object, QEvent* event);
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const;
  QSize sizeHint(const QStyleOptionViewItem& option,
                 const QModelIndex& index) const;

  QSet<AutoCopyProperty> changes() const;
  void clearChanges();

protected slots:
  void setFileDialogFlag(bool);
signals:
  void sig_addEditedTask(const QString&);
protected:
  bool FileDialogFlag;
  // record a change to an item in the model.
  // this simply saves the item in the set of changes
  void recordChange(QAbstractItemModel* model, const QModelIndex& index);

  // properties changed by user via this delegate
  QSet<AutoCopyProperty> mChanges;
};

#endif
