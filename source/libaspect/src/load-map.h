#ifndef LOAD_MAP_H
#define LOAD_MAP_H

#include <string>
#include <list>

extern "C" {
  struct link_map;
  struct r_debug;
}

namespace aspcpp {

class LoadMap
{
 public:
  struct Item
  {
    std::string filename;
    unsigned long load_base;
  };
  typedef std::list<struct LoadMap::Item>::const_iterator Iterator;

  Iterator Begin (void) const;
  Iterator End (void) const;

  LoadMap GetEntriesAdded (LoadMap old) const;
  LoadMap GetEntriesDeleted (LoadMap old) const;
  LoadMap Remove (LoadMap other) const;
  bool Contains (struct Item item) const;

  static LoadMap Parse (void);

  static long GetChangeNotifierAddress (void);

 private:
  LoadMap ();
  static struct link_map *GetLinkMap (void);
  static struct r_debug *GetLdDebug(void);
  std::list<struct Item> m_items;
};

} // namespace aspcpp

#endif /* LOAD_MAP_H */
