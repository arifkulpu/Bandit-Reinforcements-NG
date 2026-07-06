#include <array>

struct ID {
    ID() = default;
    explicit ID(int a_id) : _id(a_id) {}
    int _id;
};

#define USING_AE 1
#define RELOCATION(SE, AE) (USING_AE ? AE : SE)

inline std::array<ID, 1> VTABLE_XXX{RELOCATION((ID(1)), (ID(2)))};

int main() {
    return 0;
}
