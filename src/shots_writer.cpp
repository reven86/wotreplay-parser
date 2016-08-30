#include "shots_writer.h"

using namespace wotreplay;

void shots_writer_t::init(const arena_t &arena, const std::string &mode) {
    this->initialized = true;
}

void shots_writer_t::write(std::ostream &os) {
    uint32_t vehicles = shots_map.size();
    os.write((const char *)&vehicles, sizeof(vehicles));
    for (const auto& vehicle : shots_map)
    {
        os.write((const char *)&vehicle.first, sizeof(vehicle.first));

        uint32_t shots = vehicle.second.size();
        os.write((const char *)&shots, sizeof(shots));
        for (const auto& shot : vehicle.second)
        {
            os.write((const char *)&shot.attacker_descriptor, sizeof(shot.attacker_descriptor));
            os.write((const char *)&(*shot.point.begin()), shot.point.size());
            os.write((const char *)&shot.damage_factor, sizeof(shot.damage_factor));

            uint32_t modules_count = shot.modules.size();
            os.write((const char *)&modules_count, sizeof(modules_count));
            for (const auto& module : shot.modules)
                os.write((const char *)&module, sizeof(module));
        }
    }
}

shots_writer_t::shots_writer_t()
    : filter([](const packet_t &) { return true; })
{}

void shots_writer_t::update(const game_t &game) {

    std::unordered_map<uint32_t, std::vector<uint8_t> > last_frame_modules;
    float last_modules_time = 0.0f;

    for (const auto &packet : game.get_packets()) {
        // skip empty packet
        if (!filter(packet)) continue;

        if (packet.type() == 0x08)
        {
            switch(packet.sub_type())
            {
            case 0x0D:
                {
                    // modules information usually comes before shot packets
                    uint32_t victim_id = get_field<uint32_t>(packet.get_data().begin(), packet.get_data().end(), 24);
                    uint8_t module_status = get_field<uint8_t>(packet.get_data().begin(), packet.get_data().end(), 28);
                    uint8_t module_type = get_field<uint8_t>(packet.get_data().begin(), packet.get_data().end(), 29);
                    uint32_t attacker_id = get_field<uint32_t>(packet.get_data().begin(), packet.get_data().end(), 30);

                    if ((module_status == 0x0a || module_status == 0x04 || module_status == 0x05))
                    {
                        // we are safe here to compare float values this way
                        if (packet.clock() != last_modules_time)
                        {
                            last_modules_time = packet.clock();
                            last_frame_modules.clear();
                        }

                        // not best implementation ....
                        last_frame_modules[victim_id].push_back(module_type);
                    }
                }
                break;
            case 0x07:
                {
                    uint32_t attacker_id = get_field<uint32_t>(packet.get_data().begin(), packet.get_data().end(), 24);

                    const player_t& victim = game.get_player(packet.player_id());
                    const player_t * attacker = game.get_player_or_null(attacker_id);
                    int8_t num_points = get_field<int8_t>(packet.get_data().begin(), packet.get_data().end(), 28);

                    // ignore turret and gun for now, separate shots only by vehicle id.
                    std::vector<shot_t>& shots = shots_map[get_field<uint16_t>(victim.compact_descriptor.begin(), victim.compact_descriptor.end(), 0)];
                    const uint8_t * point = reinterpret_cast<const uint8_t *>(&(*packet.get_data().begin())) + 29;
                    uint8_t damage_factor = *(packet.get_data().begin() + 29 + num_points * 8 + 1);
                    auto& modules = packet.clock() == last_modules_time ? last_frame_modules.find(packet.player_id()) : last_frame_modules.end();
                    while (num_points-- > 0)
                    {
                        shots.emplace_back(shot_t{ !attacker || attacker->compact_descriptor.empty() ? (uint16_t)0 : get_field<uint16_t>(attacker->compact_descriptor.begin(), attacker->compact_descriptor.end(), 0),
                            shot_t::point_t{*point, *(point + 1), *(point + 2), *(point + 3), *(point + 4), *(point + 5), *(point + 6), *(point + 7)}, 
                            damage_factor,  modules == last_frame_modules.end() ? std::vector<uint8_t>() : (*modules).second });
                        point += 8;
                    }
                }
                break;
            }
        }
    }
}

void shots_writer_t::finish() {
    // empty
}

void shots_writer_t::reset() {
    this->initialized = false;
}

bool shots_writer_t::is_initialized() const {
    return initialized;
}

void shots_writer_t::clear() {
    shots_map.clear();
}

void shots_writer_t::set_filter(filter_t filter) {
    this->filter = filter;
}