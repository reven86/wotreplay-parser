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
            os.write((const char *)(&(*shot.begin())), shot.size());
    }
}

shots_writer_t::shots_writer_t()
    : filter([](const packet_t &) { return true; })
{}

void shots_writer_t::update(const game_t &game) {
    for (const auto &packet : game.get_packets()) {
        // skip empty packet
        if (!filter(packet)) continue;

        if (packet.type() == 0x08 && packet.sub_type() == 0x07)
        {
            const player_t& victim = game.get_player(packet.player_id());
            int8_t num_points = get_field<int8_t>(packet.get_data().begin(), packet.get_data().end(), 28);

            // ignore turret and gun for now, separate shots only by vehicle id.
            std::vector<shot_t>& shots = shots_map[get_field<uint16_t>(victim.compact_descriptor.begin(), victim.compact_descriptor.end(), 0)];
            const uint8_t * point = reinterpret_cast<const uint8_t *>(&(*packet.get_data().begin())) + 29;
            while (num_points-- > 0)
            {
                shots.emplace_back(shot_t{ *point, *(point+1), *(point+2), *(point+3), *(point+4), *(point+5), *(point+6), *(point+7) });
                point += 8;
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