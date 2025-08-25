#pragma once

// Simple in-memory database for our CRUD operations
class ItemStore
{
private:
    struct Item
    {
        int id;
        std::string name;
        std::string description;
        double price;

        // Convert to JSON string
        std::string to_json() const
        {
            std::stringstream ss;
            ss << "{";
            ss << "\"id\": " << id << ",";
            ss << "\"name\": \"" << name << "\",";
            ss << "\"description\": \"" << description << "\",";
            ss << "\"price\": " << price;
            ss << "}";
            return ss.str();
        }
    };

    std::map<int, Item> items;
    int next_id = 1;
    std::mutex mtx; // For thread safety

public:
    // Create - returns the ID of the newly created item
    int create(const std::string &name, const std::string &description, double price)
    {
        std::lock_guard<std::mutex> lock(mtx);
        int id = next_id++;
        Item item{id, name, description, price};
        items[id] = item;
        return id;
    }

    // Read - get a specific item by ID
    Item get(int id)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (items.find(id) == items.end())
        {
            throw hh_web::web_exception(
                "Item not found",
                "NOT_FOUND",
                "ItemStore::get",
                404,
                "Not Found");
        }
        return items[id];
    }

    // Read - get all items
    std::vector<Item> get_all()
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<Item> result;
        for (const auto &[id, item] : items)
        {
            result.push_back(item);
        }
        return result;
    }

    // Update - update an existing item
    void update(int id, const std::string &name, const std::string &description, double price)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (items.find(id) == items.end())
        {
            throw hh_web::web_exception(
                "Item not found",
                "NOT_FOUND",
                "ItemStore::update",
                404,
                "Not Found");
        }
        items[id] = Item{id, name, description, price};
    }

    // Delete - remove an item
    void remove(int id)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (items.find(id) == items.end())
        {
            throw hh_web::web_exception(
                "Item not found",
                "NOT_FOUND",
                "ItemStore::remove",
                404,
                "Not Found");
        }
        items.erase(id);
    }
};

// Singleton instance of our item store
ItemStore &get_item_store()
{
    static ItemStore instance;
    return instance;
}