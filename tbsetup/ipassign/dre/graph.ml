(*
 * graph.ml - simple graph module
 * Note: tests are commented out, since this module gets included by other
 * files
 *)
type ('a, 'b) node = { node_contents : 'a;
                       mutable node_edges : ('a, 'b) edge_list;
                       mutable incident_edges : int }
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
    { node_contents = contents; node_edges = []; incident_edges = 0 };;

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
                     node_edges = []; incident_edges = 0 } in
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

(* fold_nodes: ('a, 'b) t -> ('c -> ('a, 'b) node -> 'c) -> 'c list *)
let fold_nodes graph visitor base =
    List.fold_left visitor base graph.nodes
;;

(* iterate_edges: ('a, 'b) t -> (('a, 'b) edge -> unit) -> unit *)
let iterate_edges graph visitor =
    List.iter visitor graph.edges
;;

(* map_edges: ('a, 'b) t -> (('a, 'b) edge -> 'c) -> 'c list *)
let map_edges graph visitor =
    List.map visitor graph.edges
;;

(* count_nodes: ('a, 'b) t -> int *)
let count_nodes graph =
    (* We should probably make this faster by storing the size in the graph *)
    List.length graph.nodes
;;

(* Read in one of Jon's graph files *)
let read_graph_file (filename : string) : ('a,'b) t =
    let channel = open_in filename in
    let rec get_edges () : ('a * 'a) list =
        try
            let line = input_line channel in
            let parts = Str.split (Str.regexp " +") line in
            (int_of_string (List.nth parts 2), int_of_string (List.nth parts 3))
            :: get_edges ()
        with
            End_of_file -> []
    in
    let edges = get_edges () in
    let rec make_graph_from_edges (edges : ('a * 'a) list) : ('a,'b) t =
        match edges with
          [] -> empty_graph()
        | x::xs -> let g = make_graph_from_edges xs in
            (match x with (first, second) -> 
                (* Add the verticies to the graph if they are not in there
                 * already *)
                let src =
                    if not (is_member g first) then add_node g first
                    else find_node g first in
                let dst =
                    if not (is_member g second) then add_node g second
                    else find_node g second in
                let edge = add_edge g src dst 1 in
                g)
    in
    make_graph_from_edges edges
;;

let rec eat_shit channel =
    let line = input_line channel in
    let firsttwo = Str.first_chars line 2 in
    if firsttwo = "%%" then () else eat_shit channel
;;

(* Read in one of Jon's graph files *)
let read_subgraph_file (filename : string) : ('a,'b) t =
    let channel = if filename = "-" then stdin else open_in filename in
    eat_shit channel;
    let rec get_nodes () : int list list =
        try
            let line = input_line channel in
            let parts = Str.split (Str.regexp " +") line in
            let edges = List.tl (List.tl parts) in
            let edge_ints = List.map int_of_string edges in
            edge_ints :: get_nodes ()
        with
            End_of_file -> []
    in
    let make_edges (nodes : int list list) : int list list =
        let (table : (int,int list) Hashtbl.t) = Hashtbl.create 100 in
        let invert_edges (node : int) (edges : int list) : unit =
            List.iter (fun (edge : int) ->
                if Hashtbl.mem table edge then
                    let old_nodes = Hashtbl.find table edge in
                    Hashtbl.replace table edge (node :: old_nodes)
                else
                    Hashtbl.add table edge [node]
            ) edges
        in
        let rec iter_nodes (nodes : int list list) (node : int) : unit =
            match nodes with
              [] -> ()
            | x :: xs -> invert_edges node x; iter_nodes xs (node + 1)
        in
        iter_nodes nodes 0;
        let (edge_list : int list list ref) = ref [] in
        Hashtbl.iter (fun (edge : int) (nodes : int list) ->
            edge_list := nodes :: !edge_list
        ) table;
        !edge_list
    in
    let nodes = get_nodes () in
    let g = empty_graph() in
    let rec make_graph_from_edges (edges : int list list) : unit =
        match edges with
          [] -> ()
        | x::xs ->
            make_graph_from_edges xs;
            let rec add_edges (src : int) (dsts : int list) : unit =
                match dsts with
                  [] -> ()
                | dst :: rest ->
                    (* Add the verticies to the graph if they are not in there
                     * already *)
                    let src_node =
                        if not (is_member g src) then add_node g src
                        else find_node g src in
                    let dst_node =
                        if not (is_member g dst) then add_node g dst
                        else find_node g dst in
                    let edge = add_edge g src_node dst_node 1 in
               add_edges src rest in
            let rec iter_nodes (nodes : int list) : unit =
                match nodes with
                  x :: xs -> add_edges x xs; iter_nodes xs
                | [] -> ()
            in
        iter_nodes x
    in
    let rec add_nodes (i : int) (nodes : int list list) : unit =
        match nodes with
          [] -> ()
        | x :: xs -> ignore (add_node g i); add_nodes (i + 1) xs in
    add_nodes 0 nodes;
    let edges = make_edges nodes in
    make_graph_from_edges edges;
    (* XXX - not a great way to do this *)
    (* let rec set_edge_count (which : int) (edges: int list list) : unit =
        match nodes with
          [] -> ()
        | x :: xs ->
                match x with
                  y :: [] -> ()
                |
                let node = find_node g which in
                node.incident_edges <- (List.length x);
                set_edge_count (which + 1) xs in *)
    let rec set_edge_count (edges: int list list) : unit =
        match edges with
          [] -> ()
        | x :: xs ->
                (match x with
                  [] -> ()
                | y :: [] -> ()
                | y :: ys as yss -> List.iter (fun x -> let node = find_node g x
                        in node.incident_edges <- node.incident_edges + 1) yss);
                set_edge_count xs in
    set_edge_count edges;
    g
;;

(* More operations will be added... *)
