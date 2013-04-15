declare function require(n: string): any;

declare module "net" {
    export function createServer(callback : (param: any) => any) : any;
}

declare module "http" {
    export function get(p1: any, p2: any) : any;
}

declare var __dirname;
