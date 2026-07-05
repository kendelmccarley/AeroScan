#include "appmenumodel.h"

namespace WingletUI {

static void searchMenuChildren(QHash<const AppMenuItem*, MenuMapLookup> *map, const AppMenuItem* menu, size_t parentIndex)
{
    // Assumes that submenu is non-null before call
    for (size_t i = 0; i < menu->numChildren; i++) {
        map->insert(&menu->submenu[i], {menu, parentIndex});
        if (menu->submenu[i].submenu) {
            searchMenuChildren(map, &menu->submenu[i], i);
        }
    }
}

AppMenuModel::AppMenuModel(const AppMenuItem* menuRoot, QObject *parent)
    : QAbstractItemModel{parent}, menuRoot(menuRoot)
{
    // Populate the parent hashmap
    parentMap = new QHash<const AppMenuItem*, MenuMapLookup>();

    assert(menuRoot->submenu);  // Menu root MUST have children
    searchMenuChildren(parentMap, menuRoot, 0);
}

AppMenuModel::~AppMenuModel()
{
    delete parentMap;
}

QVariant AppMenuModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        // If passed invalid index, assume they meant the root node
        if (role == Qt::DisplayRole) {
            // Return the menu root title
            return menuRoot->title;
        }
        else {
            // All other roles are null
            return {};
        }
    }
    else {
        const AppMenuItem* node = (const AppMenuItem*) index.internalPointer();

        if (role == Qt::DisplayRole) {
            return node->title;
        }
        else if (role == Qt::UserRole && node->submenu == NULL) {
            // Only has user role if not a submenu
            return node->type;
        }
        else {
            return {};
        }
    }
}

QModelIndex AppMenuModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column != 0) {
        return {};
    }

    const AppMenuItem* node = menuRoot;
    if (parent.isValid())
        node = (const AppMenuItem*) parent.internalPointer();

    if (!node->submenu) {
        // Not a submenu, can't get children
        return {};
    }

    if (!parent.isValid() && m_hiddenMainEntry >= 0) {
        // A top-level entry is hidden: map the visible row to its actual
        // position in the array, skipping over the hidden one.
        int numVisibleChildren = (int) node->numChildren - 1;
        if (row < 0 || row >= numVisibleChildren) {
            return {};
        }
        int actualRow = (row >= m_hiddenMainEntry) ? row + 1 : row;
        return createIndex(row, 0, (void*)(&node->submenu[actualRow]));
    }

    if (row < 0 || row >= (int) node->numChildren) {
        // Row not inside submenu list
        return {};
    }

    // Valid, return index to that row
    return createIndex(row, 0, (void*)(&node->submenu[row]));
}

QModelIndex AppMenuModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return {};
    }

    auto item = parentMap->find((const AppMenuItem*) index.internalPointer());
    if (item == parentMap->end()) {
        return {};
    }
    else if (item->parent == menuRoot) {
        return {};
    }
    else {
        // The parent is a top-level submenu; report its *visible* row so it
        // stays consistent with index() when an entry above it is hidden.
        int parentRow = (int) item->parentIndex;
        if (m_hiddenMainEntry >= 0 && parentRow > m_hiddenMainEntry)
            parentRow--;
        return createIndex(parentRow, 0, (void*) item->parent);
    }
}

int AppMenuModel::rowCount(const QModelIndex &parent) const
{
    // Get the node for the index
    const AppMenuItem* node = menuRoot;
    if (parent.isValid())
        node = (const AppMenuItem*) parent.internalPointer();

    if (node->submenu) {
        // If submenu defined, its a menu and return number of children
        int numVisibleChildren = node->numChildren;
        if (m_hiddenMainEntry >= 0 && !parent.isValid()) {
            // One top-level entry is hidden
            numVisibleChildren--;
        }
        return numVisibleChildren;
    }
    else {
        // If not it's a menu entry, return no children
        return 0;
    }
}

int AppMenuModel::columnCount(const QModelIndex &parent) const
{
    (void) parent;
    return 1;  // Always 1 column
}

Qt::ItemFlags AppMenuModel::flags(const QModelIndex &index) const
{
    (void) index;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void AppMenuModel::setHiddenMainEntry(int index) {
    if (m_hiddenMainEntry == index) {
        return;
    }

    emit layoutAboutToBeChanged();

    // Remap every persistent index that points at a top-level entry to its new
    // visible row (or invalidate it if it is now the hidden one). Children of
    // submenus keep their own row within the submenu, so they are unaffected.
    const QModelIndexList oldIndexes = persistentIndexList();
    m_hiddenMainEntry = index;

    for (const QModelIndex &old : oldIndexes) {
        const AppMenuItem* item = (const AppMenuItem*) old.internalPointer();
        auto it = parentMap->find(item);
        if (it == parentMap->end() || it->parent != menuRoot) {
            continue;  // not a top-level entry
        }
        int actualRow = (int) (item - menuRoot->submenu);
        if (m_hiddenMainEntry >= 0 && actualRow == m_hiddenMainEntry) {
            changePersistentIndex(old, {});  // this entry is now hidden
        }
        else {
            int visRow = actualRow;
            if (m_hiddenMainEntry >= 0 && actualRow > m_hiddenMainEntry)
                visRow--;
            changePersistentIndex(old, createIndex(visRow, 0, (void*) item));
        }
    }

    emit layoutChanged();
}

} // namespace WingletUI
