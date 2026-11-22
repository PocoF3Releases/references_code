import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from repoindex.headers import parse_header


SAMPLE = """\
#include <stdint.h>
#define API_LEVEL 3
typedef struct device device_t;
typedef int (*event_cb_t)(int event,
                          void* cookie);
typedef enum {
    MODE_OFF = 0,
    MODE_ON,
} mode_t;
enum class Color : uint8_t { RED = 1, GREEN };
struct device {
    struct hw_common common;
    int (*set_mode)(struct device* dev, mode_t mode);
    uint32_t flags;
};
typedef struct {
    int value;
} plain_t;
int device_open(const char* name, struct device** out);
static inline int device_close(struct device* dev) {
    return dev->close(dev);
}
class Engine {
  public:
    virtual status_t start(int flags);
    explicit Engine(int id);
};
"""


class HeaderTests(unittest.TestCase):
    def setUp(self):
        self.symbols, self.includes = parse_header(SAMPLE)
        self.by_name = {}
        for name, kind, decl in self.symbols:
            self.by_name.setdefault(name, set()).add(kind)

    def kinds(self, name):
        return self.by_name.get(name, set())

    def test_existing_kinds_remain(self):
        self.assertEqual([("stdint.h", 1)], self.includes)
        self.assertIn("define", self.kinds("API_LEVEL"))
        self.assertIn("struct", self.kinds("device"))
        self.assertIn("class", self.kinds("Engine"))
        self.assertIn("enum", self.kinds("Color"))

    def test_typedef_names(self):
        for name in ("device_t", "event_cb_t", "mode_t", "plain_t"):
            self.assertIn("typedef", self.kinds(name), name)

    def test_enumerators_in_multiline_and_one_line_enums(self):
        for name in ("MODE_OFF", "MODE_ON", "RED", "GREEN"):
            self.assertIn("enumerator", self.kinds(name), name)

    def test_function_pointer_fields_and_prototypes(self):
        self.assertIn("member", self.kinds("set_mode"))
        self.assertIn("function", self.kinds("device_open"))
        self.assertIn("function", self.kinds("start"))

    def test_parameters_fields_and_bodies_are_not_declarations(self):
        self.assertNotIn("hw_common", self.by_name)
        self.assertNotIn("close", self.by_name)
        self.assertNotIn("return", self.by_name)
        self.assertNotIn("device_close", self.by_name)


if __name__ == "__main__":
    unittest.main()
