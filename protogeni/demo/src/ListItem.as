package
{
  class ListItem
  {
    public function ListItem(newName : String, iconClass : String) : void
    {
      label = newName;
      data = null;
      icon = iconClass;
    }

    public var label : String;
    public var data : Object;
    public var icon : String;
  }
}
