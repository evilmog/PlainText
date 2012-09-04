#include "conversionutil.h"

#include <QDateTime>
#include <QDebug>
#include <QStringList>

#include "characterstats.h"
#include "combatmessage.h"
#include "gameobjectptr.h"
#include "realm.h"
#include "scriptfunctionmap.h"


QVariant ConversionUtil::fromVariant(QVariant::Type type, int userType, const QVariant &variant) {

    Realm *realm = Realm::instance();

    switch (type) {
        case QVariant::Bool:
        case QVariant::Int:
        case QVariant::Double:
        case QVariant::String:
            return variant;
        case QVariant::StringList:
            return variant.toStringList();
        case QVariant::DateTime:
            return QDateTime::fromMSecsSinceEpoch(variant.toLongLong());
        case QVariant::Map: {
            QVariantMap variantMap;
            QVariantMap map = variant.toMap();
            for (const QString &key : map.keys()) {
                QVariantList valueList = map[key].toList();
                if (valueList.length() == 3) {
                    QVariant::Type valueType = (QVariant::Type) valueList[0].toInt();
                    int valueUserType = valueList[1].toInt();
                    QVariant value = valueList[2];
                    variantMap[key] = fromVariant(valueType, valueUserType, value);
                } else {
                    qWarning() << "Invalid map format in key" << key;
                }
            }
            return variantMap;
        }
        case QVariant::UserType:
            if (userType == QMetaType::type("GameObjectPtr")) {
                return QVariant::fromValue(GameObjectPtr::fromString(realm, variant.toString()));
            } else if (userType == QMetaType::type("GameObjectPtrList")) {
                GameObjectPtrList pointerList;
                for (const QVariant &variant : variant.toList()) {
                    pointerList << GameObjectPtr::fromString(realm, variant.toString());
                }
                return QVariant::fromValue(pointerList);
            } else if (userType == QMetaType::type("ScriptFunctionMap")) {
                ScriptFunctionMap functionMap;
                QVariantMap variantMap = variant.toMap();
                for (const QString &key : variantMap.keys()) {
                    functionMap[key] = ScriptFunction::fromString(variantMap[key].toString());
                }
                return QVariant::fromValue(functionMap);
            } else if (userType == QMetaType::type("CharacterStats")) {
                return QVariant::fromValue(CharacterStats::fromVariantList(variant.toList()));
            } else if (userType == QMetaType::type("CombatMessageList")) {
                CombatMessageList messageList;
                for (const QVariant &variant : variant.toList()) {
                    messageList << CombatMessage::fromVariantList(variant.toList());
                }
                return QVariant::fromValue(messageList);
            }
            // fall-through
        default:
            qDebug() << "Unknown type: " << type;
            return QVariant();
    }
}

QString ConversionUtil::toJSON(const QVariant &variant) {

    switch (variant.type()) {
        case QVariant::Bool:
            return variant.toBool() ? "true" : "false";
        case QVariant::Int:
            return QString::number(variant.toInt());
        case QVariant::Double:
            return QString::number(variant.toDouble());
        case QVariant::String:
            if (variant.toString().isEmpty()) {
                return QString();
            } else {
                return jsString(variant.toString());
            }
        case QVariant::List: {
            QStringList stringList;
            for (const QVariant &variant : variant.toList()) {
                stringList << ConversionUtil::toJSON(variant);
            }
            if (stringList.isEmpty()) {
                return QString();
            } else {
                return "[ " + stringList.join(", ") + " ]";
            }
        }
        case QVariant::StringList: {
            QStringList stringList;
            for (const QString &string : variant.toStringList()) {
                stringList << jsString(string);
            }
            if (stringList.isEmpty()) {
                return QString();
            } else {
                return "[ " + stringList.join(", ") + " ]";
            }
        }
        case QVariant::DateTime:
            return QString::number(variant.toDateTime().toMSecsSinceEpoch());
        case QVariant::Map: {
            QStringList stringList;
            QVariantMap map = variant.toMap();
            for (const QString &key : map.keys()) {
                QVariant value = map[key];
                stringList << QString("%1: [ %2, %3, %4 ]").arg(jsString(key),
                                                                QString::number(value.type()),
                                                                QString::number(value.userType()),
                                                                toJSON(value));
            }
            return stringList.isEmpty() ? QString() : "{ " + stringList.join(", ") + " }";
        }
        case QVariant::UserType:
            if (variant.userType() == QMetaType::type("GameObject *")) {
                return variant.value<GameObject *>()->toJSON(IncludeId);
            } else if (variant.userType() == QMetaType::type("GameObjectPtr")) {
                return jsString(variant.value<GameObjectPtr>().toString());
            } else if (variant.userType() == QMetaType::type("GameObjectPtrList")) {
                QStringList stringList;
                for (const GameObjectPtr &pointer : variant.value<GameObjectPtrList>()) {
                    stringList << jsString(pointer.toString());
                }
                return stringList.isEmpty() ? QString() : "[ " + stringList.join(", ") + " ]";
            } else if (variant.userType() == QMetaType::type("ScriptFunctionMap")) {
                QStringList stringList;
                ScriptFunctionMap functionMap = variant.value<ScriptFunctionMap>();
                for (const QString &key : functionMap.keys()) {
                    stringList << QString("%1: %2").arg(jsString(key),
                                                        jsString(functionMap[key].toString()));
                }
                return stringList.isEmpty() ? QString() : "{ " + stringList.join(", ") + " }";
            } else if (variant.userType() == QMetaType::type("CharacterStats")) {
                return variant.value<CharacterStats>().toString();
            } else if (variant.userType() == QMetaType::type("CombatMessageList")) {
                QStringList stringList;
                for (const CombatMessage &message : variant.value<CombatMessageList>()) {
                    stringList << message.toString();
                }
                return stringList.isEmpty() ? QString() : "[ " + stringList.join(", ") + " ]";
            }
            // fall-through
        default:
            qDebug() << "Unknown type: " << variant.type();
            return QString();
    }
}

QString ConversionUtil::toUserString(const QVariant &variant) {

    if (!variant.isValid()) {
        return "(not set)";
    }

    switch (variant.type()) {
        case QVariant::Bool:
            return variant.toBool() ? "true" : "false";
        case QVariant::Int:
            return QString::number(variant.toInt());
        case QVariant::Double:
            return QString::number(variant.toDouble());
        case QVariant::String: {
            QString string = variant.toString();
            return string.isEmpty() ? "(empty string)" : string;
        }
        case QVariant::StringList:
            return variant.toStringList().join("\n");
        case QVariant::DateTime:
            return variant.toDateTime().toString();
        case QVariant::Map: {
            QStringList stringList;
            QVariantMap map = variant.toMap();
            for (const QString &key : map.keys()) {
                stringList << QString("%1: %2").arg(jsString(key), toUserString(map[key]));
            }
            return stringList.isEmpty() ? "(empty)" : stringList.join("\n");
        }
        case QVariant::UserType:
            if (variant.userType() == QMetaType::type("GameObjectPtr")) {
                GameObjectPtr pointer = variant.value<GameObjectPtr>();
                if (pointer.isNull()) {
                    return "(not set)";
                } else {
                    return pointer.toString() + " (" + pointer->name() + ")";
                }
            } else if (variant.userType() == QMetaType::type("GameObjectPtrList")) {
                QStringList stringList;
                for (GameObjectPtr pointer : variant.value<GameObjectPtrList>()) {
                    stringList << pointer.toString();
                }
                return "[ " + stringList.join(", ") + " ]";
            } else if (variant.userType() == QMetaType::type("CharacterStats")) {
                CharacterStats stats = variant.value<CharacterStats>();
                return QString("[ %1, %2, %3, %4, %5, %6 ]")
                       .arg(stats.strength).arg(stats.dexterity)
                       .arg(stats.vitality).arg(stats.endurance)
                       .arg(stats.intelligence).arg(stats.faith);
            }
            // fall-through
        default:
            return "(unknown type)";
    }
}

QString ConversionUtil::jsString(QString string) {

    return "\"" + string.replace('\\', "\\\\")
                        .replace('"', "\\\"")
                        .replace('\n', "\\n") + "\"";
}
