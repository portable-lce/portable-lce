#include "AttributeModifier.h"

#include <assert.h>
#include <wchar.h>

#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/util/HtmlString.h"
#include "minecraft/world/entity/ai/attributes/Attribute.h"

void AttributeModifier::_init(eMODIFIER_ID id, const std::string name,
                              double amount, int operation) {
    assert(operation < TOTAL_OPERATIONS);
    this->amount = amount;
    this->operation = operation;
    this->name = name;
    this->id = id;
    this->serialize = true;
}

AttributeModifier::AttributeModifier(double amount, int operation) {
    // Create an anonymous attribute
    _init(eModifierId_ANONYMOUS, name, amount, operation);
}

AttributeModifier::AttributeModifier(eMODIFIER_ID id, double amount,
                                     int operation) {
    _init(id, name, amount, operation);

    // Validate.notEmpty(name, "Modifier name cannot be empty");
    // Validate.inclusiveBetween(0, TOTAL_OPERATIONS - 1, operation, "Invalid
    // operation");
}

eMODIFIER_ID AttributeModifier::getId() { return id; }

std::string AttributeModifier::getName() { return name; }

int AttributeModifier::getOperation() { return operation; }

double AttributeModifier::getAmount() { return amount; }

bool AttributeModifier::isSerializable() { return serialize; }

AttributeModifier* AttributeModifier::setSerialize(bool serialize) {
    this->serialize = serialize;
    return this;
}

bool AttributeModifier::equals(AttributeModifier* modifier) {
    if (this == modifier) return true;
    if (modifier == nullptr)
        return false;  //|| getClass() != o.getClass()) return false;

    if (id != modifier->id) return false;

    return true;
}

std::string AttributeModifier::toString() {
    return "";

    /*return "AttributeModifier{" +
    "amount=" + amount +
    ", operation=" + operation +
    ", name='" + name + '\'' +
    ", id=" + id +
    ", serialize=" + serialize +
    '}';*/
}

HtmlString AttributeModifier::getHoverText(eATTRIBUTE_ID attribute) {
    double amount = getAmount();
    double displayAmount;

    if (getOperation() == AttributeModifier::OPERATION_MULTIPLY_BASE ||
        getOperation() == AttributeModifier::OPERATION_MULTIPLY_TOTAL) {
        displayAmount = getAmount() * 100.0f;
    } else {
        displayAmount = getAmount();
    }

    eMinecraftColour color;

    if (amount > 0) {
        color = eHTMLColor_9;
    } else if (amount < 0) {
        displayAmount *= -1;
        color = eHTMLColor_c;
    }

    bool percentage = false;
    switch (getOperation()) {
        case AttributeModifier::OPERATION_ADDITION:
            percentage = false;
            break;
        case AttributeModifier::OPERATION_MULTIPLY_BASE:
        case AttributeModifier::OPERATION_MULTIPLY_TOTAL:
            percentage = true;
            break;
        default:
            // No other operations
            assert(0);
    }

    char formatted[256];
    snprintf(formatted, 256, "%s%d%s %s", (amount > 0 ? "+" : "-"),
             (int)displayAmount, (percentage ? "%" : ""),
             gameServices().getString(Attribute::getName(attribute)));

    return HtmlString(formatted, color);
}