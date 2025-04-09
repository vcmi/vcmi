#include "playerselectionwidget.h"
#include "ui_playerselectionwidget.h"
#include <QString>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>


PlayerSelectionWidget::PlayerSelectionWidget(const std::set<PlayerColor>& currentSelection, QWidget* parent)
	: QDialog(parent), ui(new Ui::PlayerSelectionWidget)
{
    ui->setupUi(this);
    colorCheckboxes = {
        { PlayerColor::ALL_PLAYERS()[0], ui->checkBoxRed },
        { PlayerColor::ALL_PLAYERS()[1], ui->checkBoxBlue },
        { PlayerColor::ALL_PLAYERS()[2], ui->checkBoxTan },
        { PlayerColor::ALL_PLAYERS()[3], ui->checkBoxGreen },
        { PlayerColor::ALL_PLAYERS()[4], ui->checkBoxOrange },
        { PlayerColor::ALL_PLAYERS()[5], ui->checkBoxPurple },
        { PlayerColor::ALL_PLAYERS()[6], ui->checkBoxTeal },
        { PlayerColor::ALL_PLAYERS()[7], ui->checkBoxPink }
    };

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &PlayerSelectionWidget::editingFinished);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &PlayerSelectionWidget::cancelEditing);

	// Initialize checkboxes
	setSelectedPlayers(currentSelection);
}

PlayerSelectionWidget::~PlayerSelectionWidget()
{
    delete ui;
}

std::set<PlayerColor> PlayerSelectionWidget::selectedPlayers() const
{
	std::set<PlayerColor> selected;
	for (auto it = colorCheckboxes.begin(); it != colorCheckboxes.end(); ++it)
        if (it.value()->isChecked())
            selected.insert(it.key());
	return selected;
}

void PlayerSelectionWidget::setSelectedPlayers(const std::set<PlayerColor>& players)
{
	for (auto it = players.begin(); it != players.end(); ++it)
        if(it->isValidPlayer())
            colorCheckboxes[*it]->setChecked(true);
}

PlayerSelectionDelegate::PlayerSelectionDelegate(std::set<PlayerColor>& p)
    : BaseInspectorItemDelegate(), colors(p) 
{
}

QWidget* PlayerSelectionDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // Create the PlayerSelectionWidget editor and pass the current set of colors
    auto* editor = new PlayerSelectionWidget(colors, parent);

    // Connect the signals for when editing is finished
    connect(editor, &PlayerSelectionWidget::editingFinished, this, [this, editor]() {
        // Commit the data and close the editor when editing is done
        const_cast<PlayerSelectionDelegate*>(this)->commitData(editor);
        const_cast<PlayerSelectionDelegate*>(this)->closeEditor(editor);
    });

    return editor;
}

// Set the editor's data
void PlayerSelectionDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* dlg = qobject_cast<PlayerSelectionWidget*>(editor);
    if (!dlg)
        return;

    // Extract the player colors from the model (comma-separated list in this example)
    QStringList list = index.data(Qt::EditRole).toString().split(",", Qt::SkipEmptyParts);
    std::set<PlayerColor> selectedColors;
    for (const QString& colorString : list)
    {
        // Assuming PlayerColor has a way to decode/parse from string
        selectedColors.insert(static_cast<PlayerColor>(PlayerColor::decode(colorString.toStdString())));
    }

    // Set the selected colors in the editor
    dlg->setSelectedPlayers(selectedColors);
}

// Set the model's data based on the editor's state
void PlayerSelectionDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* dlg = qobject_cast<PlayerSelectionWidget*>(editor);
    QString tooltip = QObject::tr("Available for:\n");
    if (!dlg)
        return;

    // Retrieve the selected players from the editor
    const std::set<PlayerColor>& selected = dlg->selectedPlayers();
    QStringList colorNames;

    for (PlayerColor color : selected)
        // Assuming PlayerColor has an encode function to convert to string
        colorNames << QString::fromStdString(PlayerColor::encode(color));
    
    tooltip += colorNames.join("\n");
    // Set the data in the model
    model->setData(index, colorNames.join(","), Qt::EditRole);
    model->setItemData(index, {{Qt::ToolTipRole, QVariant{tooltip}}});
}