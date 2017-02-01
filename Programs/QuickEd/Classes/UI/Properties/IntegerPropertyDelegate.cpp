#include "IntegerPropertyDelegate.h"
#include <QSpinBox>
#include <QLayout>
#include "PropertiesModel.h"
#include "PropertiesTreeItemDelegate.h"
#include "Utils/QtDavaConvertion.h"

IntegerPropertyDelegate::IntegerPropertyDelegate(PropertiesTreeItemDelegate* delegate)
    : BasePropertyDelegate(delegate)
{
}

IntegerPropertyDelegate::~IntegerPropertyDelegate()
{
}

QWidget* IntegerPropertyDelegate::createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    QSpinBox* spinBox = new QSpinBox(parent);
    spinBox->setObjectName(QString::fromUtf8("spinBox"));
    connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(OnValueChanged()));

    return spinBox;
}

void IntegerPropertyDelegate::setEditorData(QWidget* rawEditor, const QModelIndex& index) const
{
    QSpinBox* editor = rawEditor->findChild<QSpinBox*>("spinBox");

    editor->blockSignals(true);
    DAVA::Any variant = index.data(Qt::EditRole).value<DAVA::Any>();
    editor->setMinimum(-99999);
    editor->setMaximum(99999);
    if (variant.CanGet<DAVA::int8>())
    {
        editor->setValue(variant.Get<DAVA::int8>());
    }
    else if (variant.CanGet<DAVA::uint8>())
    {
        editor->setMinimum(0);
        editor->setValue(variant.Get<DAVA::uint8>());
    }
    else if (variant.CanGet<DAVA::int16>())
    {
        editor->setValue(variant.Get<DAVA::int16>());
    }
    else if (variant.CanGet<DAVA::uint16>())
    {
        editor->setMinimum(0);
        editor->setValue(variant.Get<DAVA::uint16>());
    }
    else if (variant.CanGet<DAVA::int32>())
    {
        editor->setValue(variant.Get<DAVA::int32>());
    }
    else if (variant.CanGet<DAVA::uint32>())
    {
        editor->setMinimum(0);
        editor->setValue(variant.Get<DAVA::uint32>());
    }
    else if (variant.CanGet<DAVA::int64>())
    {
        editor->setValue(variant.Get<DAVA::int64>());
    }
    else if (variant.CanGet<DAVA::uint64>())
    {
        editor->setMinimum(0);
        editor->setValue(variant.Get<DAVA::uint64>());
    }

    editor->blockSignals(false);
    BasePropertyDelegate::SetValueModified(editor, false);
}

bool IntegerPropertyDelegate::setModelData(QWidget* rawEditor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (BasePropertyDelegate::setModelData(rawEditor, model, index))
        return true;

    QSpinBox* editor = rawEditor->findChild<QSpinBox*>("spinBox");

    DAVA::Any value = index.data(Qt::EditRole).value<DAVA::Any>();

    if (value.CanGet<DAVA::int8>())
    {
        value.Set<DAVA::int8>(editor->value());
    }
    else if (value.CanGet<DAVA::uint8>())
    {
        value.Set<DAVA::uint8>(editor->value());
    }
    else if (value.CanGet<DAVA::int16>())
    {
        value.Set<DAVA::int16>(editor->value());
    }
    else if (value.CanGet<DAVA::uint16>())
    {
        value.Set<DAVA::uint16>(editor->value());
    }
    else if (value.CanGet<DAVA::int32>())
    {
        value.Set<DAVA::int32>(editor->value());
    }
    else if (value.CanGet<DAVA::uint32>())
    {
        value.Set<DAVA::uint32>(editor->value());
    }
    else if (value.CanGet<DAVA::int64>())
    {
        value.Set<DAVA::int64>(editor->value());
    }
    else if (value.CanGet<DAVA::uint64>())
    {
        value.Set<DAVA::uint64>(editor->value());
    }

    QVariant variant;
    variant.setValue<DAVA::Any>(value);

    return model->setData(index, variant, Qt::EditRole);
}

void IntegerPropertyDelegate::OnValueChanged()
{
    QWidget* spinBox = qobject_cast<QWidget*>(sender());
    if (!spinBox)
        return;

    QWidget* editor = spinBox->parentWidget();
    if (!editor)
        return;

    BasePropertyDelegate::SetValueModified(editor, true);
    itemDelegate->emitCommitData(editor);
}
