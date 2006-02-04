type 'a t = None | Node of 'a node and
'a node = { mutable prev : 'a t; mutable next : 'a t; contents : 'a };;

let rec iter (linklist : 'a t) (visitor : ('a -> unit)) : unit =
    match linklist with
      None -> ()
    | Node(x) -> visitor x.contents; iter x.next visitor
;;

let rec rev_iter (linklist : 'a t) (visitor : ('a -> unit)) : unit =
    match linklist with
      None -> ()
    | Node(x) -> visitor x.contents; iter x.prev visitor
;;
