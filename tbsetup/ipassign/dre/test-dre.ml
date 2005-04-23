(*
 * test-dijk.ml
 * Test functions for my Dijkstra's shortest path implementation
 *)

(* Hmm, this is awkward, I have to declare all of these types even though they
 * are related. There's probably some better way to do this *)
type mygraph = (int, int) Graph.t;;
type mynode = (int, int) Graph.node;;
type myedge = (int, int) Graph.edge;;

type edge = int * int;;

(* Get a list of edges from a channel *)
let rec (get_edges : in_channel -> edge list) = function channel ->
    try
        let line = input_line channel in
        let parts = Str.split (Str.regexp " +") line in
        (int_of_string (List.nth parts 2), int_of_string (List.nth parts 3))
            :: get_edges channel
    with
        End_of_file -> []
;;

(* Read in one of Jon's graph files *)
let (read_graph_file : string -> edge list) = function filename ->
    let channel = open_in filename in
    get_edges channel
;;

(* Make a graph from and edge_list *)
let rec (make_graph_from_edges : edge list -> mygraph) = function edges ->
    match edges with
      [] -> Graph.empty_graph()
    | x::xs -> let g = make_graph_from_edges xs in
        (match x with (first, second) -> 
            (* Add the verticies to the graph if they are not in there
             * already *)
            let src =
                if not (Graph.is_member g first) then Graph.add_node g first
                    else Graph.find_node g first in
            let dst =
                if not (Graph.is_member g second) then Graph.add_node g second
                    else Graph.find_node g second in
            let edge = Graph.add_edge g src dst 1 in
        g)
;;

let rec print_weights (channel : out_channel) (weights : int array)
                      (index : int) : unit =
    if index >= Array.length weights then ()
    else (output_string channel (string_of_int index ^ ": " ^ string_of_int
        weights.(index) ^ "\n");
        print_weights channel weights (index + 1))
;;

let rec print_pred (channel : out_channel) (preds : (int, 'b) Graph.node array)
                   (index : int) : unit =
    if index >= Array.length preds then ()
    else (output_string channel (string_of_int index ^ ": " ^ string_of_int
        preds.(index).Graph.node_contents ^ "\n");
        print_pred channel preds (index + 1))
;;

let rec compute_all_dre (g : ('a, 'b) Graph.t) : float array array =
    let hops = Array.make_matrix (Graph.count_nodes g) (Graph.count_nodes g) Dijkstra.NoHop in
    let fill_array (base : unit) (node : (int, 'a) Graph.node) : unit =
        let node_id = node.Graph.node_contents in
        match (Dijkstra.run_dijkstra g node) with (_,pred) ->
        hops.(node_id) <- Dijkstra.get_first_hops g pred node;
        base
    in
    Graph.fold_nodes g fill_array ();
    Dre.compute_dre hops
;;

exception NeedArg;;
if Array.length Sys.argv < 2 then raise NeedArg;;
(* print_endline "Here 1"; *)

let edges = read_graph_file Sys.argv.(1) in
(* print_endline "Here 2"; *)
let g = make_graph_from_edges edges in
(* print_endline "Here 3"; *)
let node = Graph.find_node g 0 in
(* print_endline "Here 4"; *)
(* let res = Dijkstra.run_dijkstra g node in (); *)
let dre_table = compute_all_dre g in
let print_cell (cell : float) : unit =
    print_float cell; print_string "\t" in
let print_row (row : float array) : unit =
    Array.iter print_cell row; print_newline() in
let print_dre_table (table : float array array) : unit =
    Array.iter print_row table in
print_dre_table dre_table
(* print_endline "Here 5"; *)
(* match res with (weights,preds) ->
print_weights stdout weights 0;
print_pred stdout preds 0;; *)
