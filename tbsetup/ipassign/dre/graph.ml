(*
 * graph.ml - simple graph module
 * Note: tests are commented out, since this module gets included by other
 * files
 *)
type ('a, 'b) node = { node_contents : 'a;
                       mutable node_edges : ('a, 'b) edge_list }
and ('a, 'b) edge_list = ('a, 'b) edge list
and ('a, 'b) edge = { src : ('a, 'b) node;
                      dst : ('a, 'b) node;
                      contents : 'b };;
type ('a, 'b) node_list = ('a, 'b) node list;;

(* The main type exported by this module *)
type ('a, 'b) t = { mutable nodes : ('a, 'b) node_list;
                    mutable edges : ('a, 'b) edge_list;
                    mutable nodehash : ('a, ('a,'b) node) Hashtbl.t;
                    };;

(* empty_graph : unit -> ('a, 'b) t *)
let empty_graph () = { nodes = []; edges = []; nodehash = Hashtbl.create 127 };;

let empty_node contents =
    { node_contents = contents; node_edges = [] };;

(* Note: If this gets used a lot, speed it up by putting a hashtable
 * indexed by node contents *)
exception NoSuchNode;;
(* find_node_helper: ('a, 'b) node_list -> 'a -> ('a, 'b) node *)
let rec find_node_helper nodes contents  =
    match nodes with
      [] -> raise NoSuchNode
    | x::xs -> if (x.node_contents = contents) then x
        else find_node_helper xs contents
;;
(* find_node: ('a, 'b) t -> 'a -> ('a, 'b) node *)
(* let find_node graph contents =
    find_node_helper graph.nodes contents
;; *)
let find_node graph contents =
    try Hashtbl.find graph.nodehash contents
    with Not_found -> raise NoSuchNode
;;
(* find_node { nodes = [{ node_contents = 1; node_edges = [] }];
            edges = [] } 1;; *)
(* Should be: { node_contents = [1]; node_edges = [] } *)

(* Note, this probably needs to be speeded up *)
let size graph =
    List.length graph.nodes
;;

(* Use the exception to detect nodes that are not in the graph *)
(* is_member: ('a, 'b) t -> 'a -> bool *)
let is_member graph contents =
    try let _ = find_node graph contents in true
    with NoSuchNode -> false;
;;

(* Mutates the graph *)
(* is_member: ('a, 'b) t -> 'a -> ('a, 'b) node *)
let add_node graph contents =
    let new_node = { node_contents = (contents : 'a);
                     node_edges = [] } in
    graph.nodes <- new_node :: graph.nodes;
    Hashtbl.add graph.nodehash contents new_node;
    new_node
;;
(* add_node (empty_graph()) 5;; *)
(* Should be: {node_contents = 5; node_edges = []} *)

(* Mutates the graph, and both nodes *)
(* add_edge: ('a, 'b) t -> ('a, 'b) node -> ('a, 'b) node -> 'b
 *           -> ('a, 'b) edge *)
let add_edge graph node1 node2 contents =
    let new_edge = { src = node1; dst = node2; contents = contents } in
    graph.edges <- new_edge :: graph.edges;
    node1.node_edges <- new_edge :: node1.node_edges;
    node2.node_edges <- new_edge :: node2.node_edges;
    new_edge
;;
(* let g = empty_graph() in
let n1 = add_node g 5 in
let n2 = add_node g 10 in
add_edge g n1 n2 0;; *)
(* Should be: Something that can't be printed, because of how it is
 * recursively defined *)

(* Some simple helper functions - even though they're simple, they hide
 * the list and edge representation so that we could change them later
 * if we want *)
(* iterate_nodes: ('a, 'b) t -> (('a, 'b) node -> unit) -> unit *)
let iterate_nodes graph visitor =
    List.iter visitor graph.nodes
;;

(* map_nodes: ('a, 'b) t -> (('a, 'b) node -> 'c) -> 'c list *)
let map_nodes graph visitor =
    List.map visitor graph.nodes
;;

(* iterate_edges: ('a, 'b) t -> (('a, 'b) edge -> unit) -> unit *)
let iterate_edges graph visitor =
    List.iter visitor graph.edges
;;

(* map_edges: ('a, 'b) t -> (('a, 'b) edge -> 'c) -> 'c list *)
let map_edges graph visitor =
    List.map visitor graph.edges
;;

(* More operations will be added later, of course... *)
