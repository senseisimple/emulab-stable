package
{
  public class Geni
  {
    static var ch : String = "sa";
    static var cm : String = "cm";

    public static var getCredential = new Array(ch, "GetCredential");
    public static var resolve = new Array(ch, "Resolve");
    public static var remove = new Array(ch, "Remove");
    public static var register = new Array(ch, "Register");

    public static var discoverResources = new Array(cm, "DiscoverResources");
    public static var getTicket = new Array(cm, "GetTicket");
    public static var redeemTicket = new Array(cm, "RedeemTicket");
    public static var deleteSliver = new Array(cm, "DeleteSliver");
    public static var startSliver = new Array(cm, "StartSliver");
    public static var updateSliver = new Array(cm, "UpdateSliver");
  }
}
