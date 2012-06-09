#include "settriggercommand.h"

#include "engine/util.h"


SetTriggerCommand::SetTriggerCommand(Player *character, QObject *parent) :
    AdminCommand(character, parent) {

    setDescription(QString("Set the script for some object's trigger.\n"
                           "\n"
                           "Usage: set-trigger <object-name> [#] <trigger-name> <script>\n"
                           "\n"
                           "Type %1 to see a list of all available triggers.")
                   .arg(Util::highlight("help triggers")));
}

SetTriggerCommand::~SetTriggerCommand() {
}

void SetTriggerCommand::execute(const QString &command) {

    setCommand(command);

    /*QString alias = */takeWord();

    GameObjectPtrList objects = takeObjects(currentArea()->objects());
    if (!requireUnique(objects, "Object not found.", "Object is not unique.")) {
        return;
    }

    QString triggerName = takeWord().toLower();

    if (!triggerName.startsWith("on")) {
        player()->send("Not a valid trigger name.");
        return;
    }

    QString source = takeRest();
    if (source.isEmpty()) {
        player()->send("No source for trigger.");
        return;
    }

    if (source.startsWith("function")) {
        source = "(" + source + ")";
    }

    ScriptFunction trigger = ScriptFunction::fromString(source);
    objects[0]->setTrigger(triggerName, trigger);
    player()->send(QString("Trigger %1 set.").arg(triggerName));
}