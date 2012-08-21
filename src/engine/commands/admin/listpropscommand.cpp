#include "listpropscommand.h"

#include "engine/conversionutil.h"
#include "engine/util.h"


ListPropsCommand::ListPropsCommand(Player *character, QObject *parent) :
    AdminCommand(character, parent) {

    setDescription("List all properties of an object, with their values.\n"
                   "\n"
                   "Usage: list-props <object-name> [#]");
}

ListPropsCommand::~ListPropsCommand() {
}

void ListPropsCommand::execute(const QString &command) {

    setCommand(command);

    /*QString alias = */takeWord();

    GameObjectPtrList objects = takeObjects(currentArea()->objects());
    if (!requireUnique(objects, "Object not found.", "Object is not unique.")) {
        return;
    }

    GameObjectPtr object = objects[0];
    send(QString("These are all known properties of %1:\n"
                 "\n").arg(Util::highlight(QString("object #%1").arg(object->id()))));

    const int lineLength = 45;

    for (const QMetaProperty &metaProperty : object->metaProperties()) {
        const char *name = metaProperty.name();
        if (strcmp(name, "objectType") == 0 || strcmp(name, "id") == 0) {
            continue;
        }

        QString value = ConversionUtil::toUserString(object->property(name));
        if (value.isEmpty()) {
            value = "(empty string)";
        }
        if (!metaProperty.isWritable()) {
            value += " (read-only)";
        }

        QStringList valueLines = value.split('\n');
        for (int i = 0; i < valueLines.length(); i++) {
            if (valueLines[i].length() > lineLength) {
                QStringList lines = Util::splitLines(valueLines[i], lineLength);
                valueLines.removeAt(i);
                for (int j = 0; j < lines.length(); j++, i++) {
                    valueLines.insert(i, lines[j]);
                }
            }
        }

        send("  " + Util::highlight(QString(name).leftJustified(30)) + "  " + valueLines[0]);
        for (int i = 1; i < valueLines.size(); i++) {
            send("                                  " + valueLines[i]);
        }
    }

    send("\n");
}
