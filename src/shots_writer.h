#ifndef wotreplay__shots_writer_h
#define wotreplay__shots_writer_h

#include "game.h"
#include "packet.h"
#include "writer.h"
#include <unordered_map>

namespace wotreplay {
    /**
     * Output in binary form all shots received related to each tank in the game.
     */
    class shots_writer_t : public writer_t {
    public:
        /** default constructor */
        shots_writer_t();
        virtual void update(const game_t &game) override;
        virtual void finish() override;
        virtual void reset() override;
        virtual void write(std::ostream &os) override;
        virtual bool is_initialized() const override;
        virtual void init(const arena_t &arena, const std::string &mode) override;
        virtual void clear() override;
        virtual void set_filter(filter_t filter);
    private:
        bool initialized;
        filter_t filter;

        struct shot_t
        {
            typedef std::array<uint8_t, 8> point_t;

            uint16_t attacker_descriptor;   // attacker's vehicle id
            point_t point;
            uint8_t damage_factor;  // 0 means no damage at all (no pen, ricochet, or damaged external module)

            std::vector<uint8_t> modules;   // damaged or destroyed modules
        };

        std::unordered_map<uint16_t, std::vector<shot_t> > shots_map;
    };
}

#endif // wotreplay__shots_writer_h
